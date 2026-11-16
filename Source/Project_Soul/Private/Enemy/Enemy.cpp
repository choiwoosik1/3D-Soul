#include "Enemy/Enemy.h"
#include "Enemy/EnemyDamageTypes.h"
#include "Enemy/PlayerSnapshot.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemy::AEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    RightWeaponHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWeaponHitbox"));
	LeftWeaponHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWeaponHitbox"));
}

// Called after the actor's components have been initialized
void AEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents();

    // Attach the weapon hitbox to the character's weapon socket
    RightWeaponHitbox->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightWeaponSocketName);
    RightWeaponHitbox->SetCollisionObjectType(ECC_WorldDynamic);
    RightWeaponHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    RightWeaponHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    RightWeaponHitbox->SetGenerateOverlapEvents(true);
    RightWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    LeftWeaponHitbox->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, LeftWeaponSocketName);
    LeftWeaponHitbox->SetCollisionObjectType(ECC_WorldDynamic);
    LeftWeaponHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    LeftWeaponHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    LeftWeaponHitbox->SetGenerateOverlapEvents(true);
    LeftWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Initialize health, poise, character state, movement settings
	CurrentHealth = MaxHealth;

    LimitPoise = MaxPoise;
    CurrentPoise = LimitPoise;

	SetCharacterState(EEnemyState::Idle);

    GetCharacterMovement()->MaxAcceleration = 4000.f;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;
    bUseControllerRotationYaw = false;

	InitialTransform = GetActorTransform();
}

// Update the character's state and adjust movement and blackboard values accordingly
void AEnemy::SetCharacterState(EEnemyState NewState)
{
    if (CharacterState == NewState) return;
    CharacterState = NewState;

    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
    if (!BBComp) return;

	// Handle different character states and adjust movement speed, rotation rate, and blackboard values
    switch (CharacterState)
    {
        case EEnemyState::Idle:
        case EEnemyState::Patrol:
            GetWorldTimerManager().ClearTimer(DecisionTimerHandle);
            GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
			GetCharacterMovement()->RotationRate = FRotator(0.f, 180.f, 0.f);
            break;
        case EEnemyState::InCombat:
            StartDecisionTimer();
            GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
            BBComp->SetValueAsBool(FName("IsInAction"), false);
            break;
        case EEnemyState::Attack:
            GetWorldTimerManager().ClearTimer(DecisionTimerHandle);
            AIC->StopMovement();
            BBComp->SetValueAsBool(FName("IsInAction"), true);
            BBComp->ClearValue(FName("Destination"));
            break;
        case EEnemyState::Stagger:
        case EEnemyState::Groggy:
        case EEnemyState::Dead:
            GetWorldTimerManager().ClearTimer(DecisionTimerHandle);
            GetCharacterMovement()->MaxWalkSpeed = 0.f;
            GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 0.f);
            AIC->ClearFocus(EAIFocusPriority::Gameplay);
            BBComp->SetValueAsBool(FName("IsInAction"), true);
            BBComp->ClearValue(FName("Destination"));
            break;
    }
}

// Adjust the character's movement speed in combat state based on the distance to the target
void AEnemy::SetSpeedByDistance(float Distance)
{
	// If the distance is less than or equal to the maximum combat range, set the speed to StrafeSpeed
    if (Distance <= MaxCombatRange)
    {
        GetCharacterMovement()->MaxWalkSpeed = StrafeSpeed;
    }
	// If the distance is between 1.0 and 1.2 times the maximum combat range, interpolate the speed between StrafeSpeed and WalkSpeed
	else if (Distance <= 1.2f * MaxCombatRange)
    {
        GetCharacterMovement()->MaxWalkSpeed = FMath::GetMappedRangeValueClamped(
            FVector2D(MaxCombatRange, 1.2f * MaxCombatRange), FVector2D(StrafeSpeed, WalkSpeed), Distance);
    }
	// If the distance is between 1.2 and 1.5 times the maximum combat range, interpolate the speed between WalkSpeed and RunSpeed
	else if (Distance <= 1.5f * MaxCombatRange)
    {
        GetCharacterMovement()->MaxWalkSpeed = FMath::GetMappedRangeValueClamped(
            FVector2D(1.2f * MaxCombatRange, 1.5f * MaxCombatRange), FVector2D(WalkSpeed, RunSpeed), Distance);
    }
	// If the distance is greater than 1.5 times the maximum combat range, set the speed to RunSpeed
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
    }
}

// Handle incoming damage, update health and poise,
// And determine if the enemy should stagger, enter groggy state, or die
// Handle incoming damage, update health and poise,
// And determine if the enemy should stagger, enter groggy state, or die
float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (CharacterState == EEnemyState::Dead) return 0.f;

    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (bIsInvincible) return 0.f;

    // Being hit while waiting means the player attacked first; record it right away
    if (PlayerActionRecord.Wait.bActive)
    {
        PlayerActionRecord.Wait.bAttackedFirst = true;
        CloseWait();
    }

    // A hit this soon after a pattern started was already on its way, so it says nothing about the pattern
    constexpr float PunishIgnoreTime = 0.5f;
    if (GetWorld()->GetTimeSeconds() - AttackStartTime >= PunishIgnoreTime)
    {
        ClosePunishWatch(true);
    }

    // Apply backstab or critical hit multipliers if applicable, and reset the flags
    if (bBackstabbed)
    {
        if (DamageCauser != AttackInitiator) return 0.f;
        ActualDamage *= BackstabMultiplier;
        bBackstabbed = false;
        bIsInvincible = true;
    }
    else if (bCriticalHit)
    {
        if (DamageCauser != AttackInitiator) return 0.f;
        ActualDamage *= CriticalHitMultiplier;
        bCriticalHit = false;
        bIsInvincible = true;
    }

    // Reduce health by the actual damage amount
    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	// Notify the health bar UI to update based on the new health value
	NotifyHealthBarUpdate(CurrentHealth, MaxHealth);

    // UE_LOG(LogTemp, Warning, TEXT("Hit Enemy: %s | Damage: %f | Health: %f"), *GetName(), ActualDamage, CurrentHealth);

	// Check if health has dropped to zero or below, and if so, trigger the death logic
    if (CurrentHealth <= 0.0f)
    {
        Die();
        return ActualDamage;
    }

    // Poise reduction logic
    if (CurrentPoise > 0)
    {
        CurrentPoise -= DamageAmount;
        if (CurrentPoise <= 0)
        {
            // Reduce Limit Poise
            float PoiseReduction = MaxPoise * LimitPoiseReductionRate;
            LimitPoise = FMath::Clamp(LimitPoise - PoiseReduction, 0.f, MaxPoise);

            // Enter stagger or groggy state based on remaining Limit Poise
            if (LimitPoise <= MaxPoise * PoiseThreshold)
            {
                EnterGroggy();
            }
            else
            {
                EnterStagger();
            }
        }
    }

    return ActualDamage;
}

// Timer function to start the decision-making process for the enemy in combat state
void AEnemy::StartDecisionTimer()
{
    GetWorldTimerManager().SetTimer(DecisionTimerHandle, this, &AEnemy::OnDecisionTimer, DecisionInterval, false);
}

// Timer callback function to decide the enemy's next action in combat state
void AEnemy::OnDecisionTimer()
{
    if (CharacterState == EEnemyState::InCombat)
    {
        DecideNextAction();
        StartDecisionTimer();
        DrawBehaviorDebug();
    }
}

// Base decision logic for the enemy's next action
void AEnemy::DecideNextAction()
{
    if (CharacterState != EEnemyState::InCombat) return;

    AActor* Target = GetCombatTarget();
    if (!Target) return;

    const float Now = GetWorld()->GetTimeSeconds();
    const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

    // An attack counts as started on the first decision that sees it
    const bool bTargetAttacking = FPlayerSnapshot::Capture(Target).Action == EPlayerAction::Attacking;
    const bool bAttackStarted = bTargetAttacking && !PlayerActionRecord.bTargetWasAttacking;
    PlayerActionRecord.bTargetWasAttacking = bTargetAttacking;

    FWaitObservation& Wait = PlayerActionRecord.Wait;
    if (Wait.bActive && bTargetAttacking)
    {
        Wait.bAttackedFirst = true;
    }

    // Punish: answer the target's attack with one of the fastest patterns. Rolled once per attack.
    if (bAttackStarted && FMath::FRand() < GetPunishChance())
    {
        const int32 PunishIdx = PickPattern(Distance, true);
        if (PunishIdx != INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("Punish P=%d Chance=%.2f"), PunishIdx, GetPunishChance());
            PerformAttackPattern(PunishIdx);
            PlayerActionRecord.bPunishAttack = true;
            return;
        }
    }

    // Leave a pause after the previous pattern
    if (Now - LastAttackEndTime < MinAttackGap) return;

    const int32 SelectedIdx = PickPattern(Distance, false);

    if (Wait.bActive)
    {
        if (Now < Wait.EndTime) return;

        // The wait is over. It stays open through the windup of the attack that follows,
        // so with nothing to attack with it is closed here instead.
        if (SelectedIdx == INDEX_NONE)
        {
            CloseWait();
        }
    }
    else if (SelectedIdx != INDEX_NONE && FMath::FRand() < GetWaitChance())
    {
        // Hold the attack for a moment and watch what the target does
        Wait.bActive = true;
        Wait.EndTime = Now + FMath::FRandRange(WaitDuration.Min, WaitDuration.Max);
        UE_LOG(LogTemp, Warning, TEXT("WaitStart Chance=%.2f"), GetWaitChance());
        return;
    }

    if (SelectedIdx != INDEX_NONE)
    {
        PerformAttackPattern(SelectedIdx);
    }
}

// Execute the decided attack pattern
void AEnemy::PerformAttackPattern(int32 AttackPatternIdx)
{
    if (CharacterState != EEnemyState::InCombat) return;
    if (!Patterns.IsValidIndex(AttackPatternIdx)) return;

    // Whoever starts a punish sets the flag again after this call
    PlayerActionRecord.bPunishAttack = false;

    // Decisions stop while attacking, so the next one must judge the target's attack afresh
    PlayerActionRecord.bTargetWasAttacking = false;

    // From here until shortly after the pattern ends, being hit counts against this pattern
    ClosePunishWatch(false);
    PunishWatchIdx = AttackPatternIdx;

    SetCharacterState(EEnemyState::Attack);
    PatternIdx = AttackPatternIdx;
    AttackIdx = 0;

    // A new combo starts without a previous dodge
    PlayerActionRecord.PatternPrevBucket = INDEX_NONE;
    PlayerActionRecord.PendingSample.bActive = false;

    // Start watching the player for the first swing of this pattern
    PlayerActionRecord.Swing = FSwingObservation();
    AttackStartTime = GetWorld()->GetTimeSeconds();
    GetWorldTimerManager().SetTimer(SwingObserveTimerHandle, this, &AEnemy::ObserveSwing, 0.1f, true);

    // Play the attack montage associated with the selected attack pattern
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && Patterns[PatternIdx].Montage)
    {
        AnimInstance->Montage_Play(Patterns[PatternIdx].Montage);
    }
}

// Reset the enemy's state to InCombat after completing an attack pattern
void AEnemy::FinishAttackPattern()
{
    if (CharacterState != EEnemyState::Attack) return;

    // Patterns without a weapon hitbox never reach the point where a wait is normally closed
    CloseWait();
    PlayerActionRecord.bPunishAttack = false;

    // Keep the punish watch open for a moment after the pattern, then record that nothing happened
    constexpr float PunishGrace = 0.3f;
    GetWorldTimerManager().SetTimer(PunishWatchTimerHandle,
        FTimerDelegate::CreateUObject(this, &AEnemy::ClosePunishWatch, false), PunishGrace, false);

    LastAttackEndTime = GetWorld()->GetTimeSeconds();
    ResumeCombat();
}

// Interrupt the current attack, disable the weapon hitbox, and prevent parrying
void AEnemy::InterruptCurrentAttack()
{
    // Discard the pending dodge sample so an interrupted attack is never learned as a dodge
    PlayerActionRecord.PendingSample.bActive = false;
    PlayerActionRecord.PatternPrevBucket = INDEX_NONE;

    // Discard the swing being judged so an interrupted swing is never recorded
    PlayerActionRecord.Swing = FSwingObservation();

    // A hit that counts as a punish was already recorded when the damage arrived.
    // Anything still open belongs to a pattern that never played out, so it is dropped without a record.
    PunishWatchIdx = INDEX_NONE;
    GetWorldTimerManager().ClearTimer(PunishWatchTimerHandle);

    // A punish stopped before its first swing resolved has failed
    if (PlayerActionRecord.bPunishAttack)
    {
        PlayerActionRecord.bPunishAttack = false;
        PlayerActionRecord.Behavior.Punish.Record(false);
        UE_LOG(LogTemp, Warning, TEXT("PunishResult Hit=0 (interrupted)"));
    }

    CloseWait();
    PlayerActionRecord.bTargetWasAttacking = false;

    bCanBeParried = false;
    DisableWeaponHitbox();
}

// Enable the weapon hitbox for the current attack
void AEnemy::EnableWeaponHitbox()
{
    // A wait that ended with this attack was kept open through the windup; it ends here
    CloseWait();

    // Open the observation first: enabling collision fires the overlap at once if the target is already inside
    PlayerActionRecord.Swing.bOpen = true;

    // The first hitbox activation tells how long this pattern takes to become dangerous
    if (AttackIdx == 0 && Patterns.IsValidIndex(PatternIdx))
    {
        Patterns[PatternIdx].OutcomeStats.StartupTime = GetWorld()->GetTimeSeconds() - AttackStartTime;
    }

    RightWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    LeftWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

// Disable the weapon hitbox after the attack ends, and learn where the target went if a lunge was tracked
void AEnemy::DisableWeaponHitbox()
{
    if (RightWeaponHitbox->GetCollisionEnabled() == ECollisionEnabled::NoCollision) return;

    RightWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;

    // Resolve the pending lunge sample: where did the target end up compared to where it stood at commit time?
    FAttackSample& Sample = PlayerActionRecord.PendingSample;
    AAIController* AIC = Cast<AAIController>(GetController());
    if (Sample.bActive && AIC && AIC->GetFocusActor())
    {
        Sample.ResolvedLocation = AIC->GetFocusActor()->GetActorLocation();

        // Log the result of the lunge attack, including pattern index, attack index, and whether any actors were hit
        // UE_LOG(LogTemp, Warning, TEXT("  P=%d A=%d Hit=%d"), PatternIdx, AttackIdx, AlreadyHitActors.Num() > 0);

        // The next lunge of this combo is predicted from this dodge
        PlayerActionRecord.PatternPrevBucket = RecordSample(Sample);
    }
    Sample.bActive = false;

    // Record how the player dealt with this swing
    FSwingObservation& Swing = PlayerActionRecord.Swing;
    if (Swing.bOpen)
    {
        const EAttackOutcome Outcome = Swing.Resolve();
        Patterns[PatternIdx].OutcomeStats.Record(Outcome);

        // Guard habit only counts swings the player had to deal with
        if (Outcome != EAttackOutcome::OutRanged)
        {
            PlayerActionRecord.Behavior.Guard.Record(Swing.bGuarded);
        }

        // The first swing decides whether a punish attempt worked
        if (PlayerActionRecord.bPunishAttack)
        {
            PlayerActionRecord.bPunishAttack = false;
            PlayerActionRecord.Behavior.Punish.Record(Outcome == EAttackOutcome::Hit);
            UE_LOG(LogTemp, Warning, TEXT("PunishResult Hit=%d"), Outcome == EAttackOutcome::Hit);
        }

        // UE_LOG(LogTemp, Warning, TEXT("SwingOutcome P=%d A=%d %s Reached=%d Rolled=%d Carried=%d"),
        //     PatternIdx, AttackIdx, LexToString(Outcome), Swing.bReached, Swing.bRolled, Swing.bRollHandled);
    }

    // The next swing of the pattern starts with a clean observation; a roll still in progress belongs to this swing
    Swing = FSwingObservation();
    Swing.bRollHandled = FPlayerSnapshot::Capture(GetCombatTarget()).Action == EPlayerAction::Rolling;

    AlreadyHitActors.Empty();
    AttackIdx++;
}

// Handle overlap events for the weapon hitbox, applying damage to valid targets
void AEnemy::OnWeaponHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;
    const FAttackProperties& Attack = Patterns[PatternIdx].Attacks[AttackIdx];

    if (!OtherActor || Cast<AEnemy>(OtherActor) || AlreadyHitActors.Contains(OtherActor)) return;
    AlreadyHitActors.Add(OtherActor);

    // Judge the swing from what the target is doing at contact, before the damage changes its state
    FSwingObservation& Swing = PlayerActionRecord.Swing;
    if (Swing.bOpen && !Swing.bReached && OtherActor == GetCombatTarget())
    {
        const FPlayerSnapshot Snapshot = FPlayerSnapshot::Capture(OtherActor);
        Swing.bReached = true;
        Swing.bGuarded = Snapshot.Action == EPlayerAction::Guarding;

        if (Snapshot.bInvincible)
        {
            // Rolled through the swing. Invincibility from anything else says nothing about this swing, so drop it.
            Swing.ContactOutcome = EAttackOutcome::Dodged;
            Swing.bOpen = Snapshot.Action == EPlayerAction::Rolling;
        }
        else if (Swing.bGuarded && Attack.GuardResponse == EGuardResponse::Block)
        {
            Swing.ContactOutcome = EAttackOutcome::Guarded;
        }
        else
        {
            Swing.ContactOutcome = EAttackOutcome::Hit;
        }
    }

    float Damage = BaseDamage * Attack.DamageMultiplier;
    TSubclassOf<UDamageType> GuardResponseType;

    // Determine the guard response type based on the attack's guard response property
    switch (Attack.GuardResponse)
    {
    case EGuardResponse::Block:
        GuardResponseType = UDamageType_Block::StaticClass();
        break;
    case EGuardResponse::GuardBreak:
        GuardResponseType = UDamageType_GuardBreak::StaticClass();
        break;
    case EGuardResponse::Ignore:
        GuardResponseType = UDamageType_Ignore::StaticClass();
        break;
    }

    UGameplayStatics::ApplyDamage(OtherActor, Damage, GetController(), this, GuardResponseType);
}

// Enable rotation during an attack, adjusting rotation speed based on the attack properties
void AEnemy::EnableAttackRotation()
{
    if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;
    const FAttackProperties& Attack = Patterns[PatternIdx].Attacks[AttackIdx];

	AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC || !AIC->GetFocusActor()) return;

	// Determine rotation direction based on the position of the target relative to the enemy
    FVector ToTarget = AIC->GetFocusActor()->GetActorLocation() - GetActorLocation();
    float Dot = FVector::DotProduct(GetActorRightVector(), ToTarget.GetSafeNormal());
    float Speed = Dot > 0.f ? Attack.RotationSpeedR : Attack.RotationSpeedL;

	GetCharacterMovement()->RotationRate = FRotator(0.f, Speed, 0.f);
}

// Disable rotation during an attack, setting rotation speed to zero
void AEnemy::DisableAttackRotation()
{
    GetCharacterMovement()->RotationRate = FRotator::ZeroRotator;
}

// Enable movement during an attack, aiming at the target position corrected by the learned dodge offset
void AEnemy::EnableAttackMovement()
{
    if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;
    const FAttackProperties& Attack = Patterns[PatternIdx].Attacks[AttackIdx];

    if (Attack.MovementSpeed <= 0.f) return;

    AAIController* AIC = Cast<AAIController>(GetController());
    AActor* Target = AIC ? AIC->GetFocusActor() : nullptr;
    if (!Target) return;

    const FVector TargetLocation = Target->GetActorLocation();
    FAttackSample& Sample = PlayerActionRecord.PendingSample;

    // The sample and the correction are decided once, on the first call of the swing
    const bool bReAim = Sample.bActive && Sample.PatternIdx == PatternIdx && Sample.AttackIdx == AttackIdx;
    if (!bReAim)
    {
        // Attack axis: horizontal direction from this enemy to the target at commit time
        const FVector Forward = (TargetLocation - GetActorLocation()).GetSafeNormal2D();

        // Predict from how the target dodged the previous lunge of this combo
        Sample.Begin(PatternIdx, AttackIdx, Forward, TargetLocation, PlayerActionRecord.PatternPrevBucket);
        Sample.AimShift = ApplyAimOffset(TargetLocation, Forward, Attack, Sample.PrevBucket) - TargetLocation;
    }

    // Re-aims track the target but add only the part of the predicted dodge it has not covered yet,
    // so a target already dodging as predicted is not corrected twice
    const FVector ShiftDir = Sample.AimShift.GetSafeNormal2D();
    const float ShiftSize = Sample.AimShift.Size2D();
    const float Covered = FVector::DotProduct(TargetLocation - Sample.Origin, ShiftDir);
    const float Remaining = FMath::Clamp(ShiftSize - Covered, 0.f, ShiftSize);
    const FVector AimLocation = TargetLocation + ShiftDir * Remaining;

    GetCharacterMovement()->MaxWalkSpeed = Attack.MovementSpeed;

    // Debug: remove after tuning
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Sample.Forward);
    // UE_LOG(LogTemp, Warning, TEXT("LungeAim P=%d A=%d ReAim=%d Prev=%d ShiftF=%.0f ShiftR=%.0f Remaining=%.0f"),
    //     PatternIdx, AttackIdx, bReAim, Sample.PrevBucket,
    //     FVector::DotProduct(Sample.AimShift, Sample.Forward), FVector::DotProduct(Sample.AimShift, Right), Remaining);

    // Debug: blue = target at commit time, green = corrected aim (remove once tuned)
    // DrawDebugSphere(GetWorld(), TargetLocation, 20.f, 12, FColor::Blue, false, 3.f);
    // DrawDebugSphere(GetWorld(), AimLocation, 20.f, 12, FColor::Green, false, 3.f);

    // The shifted aim may be off the navmesh (walls, ledges), so project it to avoid a failed move request
    AIC->MoveToLocation(AimLocation, MinCombatRange, true, true, true);
}

// Disable movement during an attack, setting walk speed to zero
void AEnemy::DisableAttackMovement()
{
    GetCharacterMovement()->MaxWalkSpeed = 0.f;
}

// Shift BaseLocation by the predicted dodge offset of this swing (only rolls the dice, changes no state)
FVector AEnemy::ApplyAimOffset(const FVector& BaseLocation, const FVector& Forward, const FAttackProperties& Attack, int32 PrevBucket) const
{
    FVector2D Offset;
    float Confidence = 0.f;
    if (!Attack.DodgeStats.Query(PlayerActionRecord.DodgeFallback, PrevBucket, Offset, Confidence)) return BaseLocation;

    // Aiming short of the target makes the lunge stop out of reach, while aiming past it still connects
    Offset.X = FMath::Max(Offset.X, 0.f);

    // Apply the correction only some of the time so the enemy never becomes fully deterministic
    if (FMath::FRand() > Confidence * MaxApplyChance) return BaseLocation;

    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
    const FVector Shift = Forward * Offset.X + Right * Offset.Y;
    return BaseLocation + Shift.GetClampedToMaxSize(MaxAimShift);
}

// Single entry point for attack outcomes: converts the target's displacement to attack-axis space
// and learns it for the swing that produced the sample. Returns the recorded dodge bucket.
int32 AEnemy::RecordSample(const FAttackSample& Sample)
{
    if (!Patterns.IsValidIndex(Sample.PatternIdx) || !Patterns[Sample.PatternIdx].Attacks.IsValidIndex(Sample.AttackIdx)) return INDEX_NONE;

    // A hit staggers the target in place, so the resolved position is the real dodge distance whether or not it landed
    const FVector Displacement = Sample.ResolvedLocation - Sample.Origin;
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Sample.Forward);
    const FVector2D Offset(FVector::DotProduct(Displacement, Sample.Forward), FVector::DotProduct(Displacement, Right));

    const int32 Bucket = Patterns[Sample.PatternIdx].Attacks[Sample.AttackIdx].DodgeStats.Record(Offset, Sample.PrevBucket);
    PlayerActionRecord.DodgeFallback.Record(Offset, Sample.PrevBucket);

    // Debug: remove after tuning (bucket: 0 Stay, 1 Left, 2 Right, 3 Away, 4 Toward)
    // UE_LOG(LogTemp, Warning, TEXT("DodgeSample F=%.0f R=%.0f Prev=%d Bucket=%d"), Offset.X, Offset.Y, Sample.PrevBucket, Bucket);

    return Bucket;
}

// Actor this enemy is currently fighting
AActor* AEnemy::GetCombatTarget() const
{
    const AAIController* AIC = Cast<AAIController>(GetController());
    return AIC ? AIC->GetFocusActor() : nullptr;
}

// Runs every 0.1 seconds during an attack pattern and notes what the player does between decisions
void AEnemy::ObserveSwing()
{
    if (CharacterState != EEnemyState::Attack)
    {
        GetWorldTimerManager().ClearTimer(SwingObserveTimerHandle);
        return;
    }

    const AActor* Target = GetCombatTarget();
    const EPlayerAction Action = FPlayerSnapshot::Capture(Target).Action;

    // An attack the player begins during the windup still counts for the wait that led to this pattern
    if (Action == EPlayerAction::Attacking && PlayerActionRecord.Wait.bActive)
    {
        PlayerActionRecord.Wait.bAttackedFirst = true;
    }

    FSwingObservation& Swing = PlayerActionRecord.Swing;
    if (Action != EPlayerAction::Rolling)
    {
        Swing.bRollHandled = false;
    }
    else if (!Swing.bRollHandled)
    {
        // Judge each roll once, when it starts: it only counts as a dodge if the target was close enough to need it
        Swing.bRollHandled = true;
        Swing.bRolled |= Patterns.IsValidIndex(PatternIdx) && GetDistanceTo(Target) <= Patterns[PatternIdx].AttackRange;
    }
}

// Selection weight of one attack pattern, based on how the player has dealt with it so far.
// 1.0 means no preference. Subclasses can multiply the result by their own factors.
float AEnemy::ScorePattern(int32 Idx) const
{
    constexpr float MinScore = 0.3f;
    constexpr float MaxScore = 2.5f;

    const FAttackPattern& Pattern = Patterns[Idx];

    // Patterns that land more often than expected are favored
    float Score = Pattern.OutcomeStats.GetHitRate(BehaviorStats::HitPrior) / BehaviorStats::HitPrior;

    // Patterns with a swing that beats guard are favored against a player who guards a lot
    const bool bBeatsGuard = Pattern.Attacks.ContainsByPredicate([](const FAttackProperties& Attack)
        {
            return Attack.GuardResponse != EGuardResponse::Block;
        });
    if (bBeatsGuard)
    {
        const float GuardRate = PlayerActionRecord.Behavior.Guard.Get(BehaviorStats::GuardPrior);
        Score *= 1.f + GuardCounterBonus * (GuardRate - BehaviorStats::GuardPrior);
    }

    // Patterns that get this enemy hit more often than its other patterns are avoided.
    // Measured against the average so a player who punishes everything does not flatten every weight.
    float MeanPunished = 0.f;
    for (const FAttackPattern& Other : Patterns)
    {
        MeanPunished += Other.OutcomeStats.Punished.Get(BehaviorStats::PunishedPrior);
    }
    MeanPunished /= Patterns.Num();
    Score *= 1.f - PunishedPenalty * (Pattern.OutcomeStats.Punished.Get(BehaviorStats::PunishedPrior) - MeanPunished);

    // Clamp first so no pattern disappears or takes over, then make the pattern used last less likely
    Score = FMath::Clamp(Score, MinScore, MaxScore);
    return Idx == PatternIdx ? Score * RepeatPenalty : Score;
}

// Show the learned statistics above the enemy until the next decision
void AEnemy::DrawBehaviorDebug()
{
    if (!bShowBehaviorDebug) return;

    const FPlayerBehaviorStats& Behavior = PlayerActionRecord.Behavior;
    FString Text = FString::Printf(TEXT("Guard %.0f%%  Aggression %.0f%%  Wait %.0f%%  Punish %.0f%%%s"),
        Behavior.Guard.Get(BehaviorStats::GuardPrior) * 100.f,
        Behavior.Aggression.Get(BehaviorStats::AggressionPrior) * 100.f,
        GetWaitChance() * 100.f, GetPunishChance() * 100.f,
        PlayerActionRecord.Wait.bActive ? TEXT("  [Waiting]") : TEXT(""));

    for (int32 i = 0; i < Patterns.Num(); i++)
    {
        const FPatternOutcomeStats& Stats = Patterns[i].OutcomeStats;
        Text += FString::Printf(TEXT("\nP%d  W%.2f  H%.1f D%.1f G%.1f O%.1f  Hit %.0f%%  Punished %.0f%%  Range x%.2f  Startup %.2fs"),
            i, ScorePattern(i), Stats.Hit, Stats.Dodged, Stats.Guarded, Stats.OutRanged,
            Stats.GetHitRate(BehaviorStats::HitPrior) * 100.f,
            Stats.Punished.Get(BehaviorStats::PunishedPrior) * 100.f, Stats.RangeScale, Stats.StartupTime);
    }

    const float Duration = GetWorldTimerManager().GetTimerRate(DecisionTimerHandle);
    DrawDebugString(GetWorld(), FVector(0.f, 0.f, 150.f), Text, this, FColor::White, Duration, true);
}

// Pick one attack pattern that can reach the target, with a chance proportional to its weight.
// bFastOnly keeps only the patterns whose measured startup is close to this enemy's fastest one.
// Returns INDEX_NONE when no pattern qualifies.
int32 AEnemy::PickPattern(float Distance, bool bFastOnly) const
{
    // Fastest measured startup among all patterns; patterns never used yet have no measurement
    float FastestStartup = MAX_FLT;
    for (const FAttackPattern& Pattern : Patterns)
    {
        const float Startup = Pattern.OutcomeStats.StartupTime;
        if (Startup > 0.f)
        {
            FastestStartup = FMath::Min(FastestStartup, Startup);
        }
    }

    // Patterns that do not qualify keep a weight of zero
    TArray<float> Weights;
    Weights.Init(0.f, Patterns.Num());
    float TotalWeight = 0.f;
    for (int32 i = 0; i < Patterns.Num(); i++)
    {
        if (Distance > Patterns[i].AttackRange) continue;

        const float Startup = Patterns[i].OutcomeStats.StartupTime;
        if (bFastOnly && (Startup <= 0.f || Startup > FastestStartup + PunishStartupMargin)) continue;

        Weights[i] = ScorePattern(i);
        TotalWeight += Weights[i];
    }
    if (TotalWeight <= 0.f) return INDEX_NONE;

    float Roll = FMath::FRandRange(0.f, TotalWeight);
    int32 SelectedIdx = INDEX_NONE;
    for (int32 i = 0; i < Patterns.Num(); i++)
    {
        if (Weights[i] <= 0.f) continue;

        SelectedIdx = i;
        Roll -= Weights[i];
        if (Roll <= 0.f) break;
    }
    return SelectedIdx;
}

// Chance to hold an attack. Equals BaseWaitChance with no data, and drops toward zero
// against a player who keeps attacking first while the enemy waits.
float AEnemy::GetWaitChance() const
{
    const float Aggression = PlayerActionRecord.Behavior.Aggression.Get(BehaviorStats::AggressionPrior);
    return BaseWaitChance * (1.f - Aggression) / (1.f - BehaviorStats::AggressionPrior);
}

// Chance to answer the player's attack with a punish; follows how often past punishes landed
float AEnemy::GetPunishChance() const
{
    constexpr float MinChance = 0.2f;
    constexpr float MaxChance = 0.9f;

    const float HitRate = PlayerActionRecord.Behavior.Punish.Get(BehaviorStats::PunishHitPrior);
    return FMath::Clamp(HitRate, MinChance, MaxChance);
}

// End the wait in progress and record whether the player attacked during it
void AEnemy::CloseWait()
{
    FWaitObservation& Wait = PlayerActionRecord.Wait;
    if (!Wait.bActive) return;

    PlayerActionRecord.Behavior.Aggression.Record(Wait.bAttackedFirst);
    UE_LOG(LogTemp, Warning, TEXT("WaitResult AttackedFirst=%d"), Wait.bAttackedFirst);

    Wait = FWaitObservation();
}

// End the punish watch and record whether the player hit the enemy during the watched pattern
void AEnemy::ClosePunishWatch(bool bPunished)
{
    // Always drop the grace timer so it can never close the watch of a later pattern
    GetWorldTimerManager().ClearTimer(PunishWatchTimerHandle);

    if (!Patterns.IsValidIndex(PunishWatchIdx)) return;

    Patterns[PunishWatchIdx].OutcomeStats.Punished.Record(bPunished);
    UE_LOG(LogTemp, Warning, TEXT("PunishedResult P=%d Punished=%d"), PunishWatchIdx, bPunished);

    PunishWatchIdx = INDEX_NONE;
}

// Drop observations left over from a previous fight. Learned statistics are kept.
void AEnemy::ResetDecisionState()
{
    PlayerActionRecord.Wait = FWaitObservation();
    PlayerActionRecord.Swing = FSwingObservation();
    PlayerActionRecord.bPunishAttack = false;
    PlayerActionRecord.bTargetWasAttacking = false;

    PunishWatchIdx = INDEX_NONE;
    GetWorldTimerManager().ClearTimer(PunishWatchTimerHandle);
}

// Handle entering patrol state, called by EnemyAIController when player is lost
void AEnemy::EnterPatrol()
{
    SetCharacterState(EEnemyState::Patrol);
}

// Handle entering alert state, called by EnemyAIController when player is detected
void AEnemy::EnterAlert()
{
    SetCharacterState(EEnemyState::Alert);
}

// Handle entering combat state
void AEnemy::EnterCombat()
{
    SetCharacterState(EEnemyState::InCombat);
    ResetDecisionState();

    if (CombatMontage)
    {
		PlayAnimMontage(CombatMontage);
    }
}

// Handle resuming combat state after stagger or groggy states
void AEnemy::ResumeCombat()
{
    SetCharacterState(EEnemyState::InCombat);
}

// Handle entering stagger state
void AEnemy::EnterStagger()
{
    SetCharacterState(EEnemyState::Stagger);
    InterruptCurrentAttack();

    if (StaggerMontage)
    {
        PlayAnimMontage(StaggerMontage);
    }
}

// Handle exiting stagger state, resetting poise
void AEnemy::ExitStagger()
{
    if (CharacterState != EEnemyState::Stagger) return;

    CurrentPoise = LimitPoise;
    ResumeCombat();
}

// Handle entering groggy state, providing player opportunity
void AEnemy::EnterGroggy()
{
    SetCharacterState(EEnemyState::Groggy);
    InterruptCurrentAttack();

    if (GroggyMontage)
    {
        PlayAnimMontage(GroggyMontage);
    }
}

// Handle exiting groggy state, resetting poise
void AEnemy::ExitGroggy()
{
    if (CharacterState != EEnemyState::Groggy) return;

    FTimerHandle InvincibleTimerHandle;
    GetWorldTimerManager().SetTimer(InvincibleTimerHandle, this, &AEnemy::DisableInvincible, 0.3f, false);
   
    LimitPoise = MaxPoise;
    CurrentPoise = LimitPoise;
    ResumeCombat();
}

// Handle being backstabbed, applying backstab multiplier
void AEnemy::GetBackstabbed(AActor* Attacker)
{
    if (bIsInvincible) return;

    SetCharacterState(EEnemyState::Groggy);
	CurrentPoise = 0.f;
    bBackstabbed = true;
	AttackInitiator = Attacker;

	// Rotate to face the attacker
    FVector Direction = GetActorLocation() - Attacker->GetActorLocation();
    Direction.Z = 0.f;
	SetActorRotation(Direction.Rotation());

    if (BackstabMontage)
    {
		PlayAnimMontage(BackstabMontage);
    }
}

// Handle being critically hit, applying critical hit multiplier
void AEnemy::GetCriticalHit(AActor* Attacker)
{
	if (bIsInvincible) return;

    SetCharacterState(EEnemyState::Groggy);
	CurrentPoise = 0.f;
	bCriticalHit = true;
    AttackInitiator = Attacker;

	// Rotate to align with the attacker
	FVector Direction = Attacker->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	SetActorRotation(Direction.Rotation());

    if (CriticalHitMontage)
    {
		PlayAnimMontage(CriticalHitMontage);
    }
}

// Handle being parried, entering groggy state and resetting poise
void AEnemy::GetParried(AActor* Attacker)
{
	SetCharacterState(EEnemyState::Groggy);
	CurrentPoise = 0.f;
	AttackInitiator = Attacker;
	
    // Rotate to align with the attacker
	FVector Direction = Attacker->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	SetActorRotation(Direction.Rotation());
	
    if (ParriedMontage)
	{
		PlayAnimMontage(ParriedMontage);
	}
}

// Handle death
void AEnemy::Die()
{
    SetCharacterState(EEnemyState::Dead);
    InterruptCurrentAttack();

    OnEnemyDied.Broadcast(this);

    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
    }

	AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC)
    {
        AIC->Destroy();
	}

    if (RightWeaponMesh)
    {
        RightWeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        RightWeaponMesh->SetSimulatePhysics(true);
        RightWeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    }

	if (LeftWeaponMesh)
	{
		LeftWeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		LeftWeaponMesh->SetSimulatePhysics(true);
		LeftWeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}

// Reset the enemy to its initial state, clearing timers, stopping animations, restoring health and poise, and respawning the controller
void AEnemy::ResetToInitialState()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
    InterruptCurrentAttack();
    ResetDecisionState();

	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		Anim->StopAllMontages(0.f);
	}

    // Class defaults hold the original values of every default subobject
    const AEnemy* Default = GetClass()->GetDefaultObject<AEnemy>();

    // Undo ragdoll (EnableRagdoll)
    GetMesh()->SetSimulatePhysics(false);
    GetMesh()->SetCollisionProfileName(Default->GetMesh()->GetCollisionProfileName());
    GetCapsuleComponent()->SetCollisionProfileName(Default->GetCapsuleComponent()->GetCollisionProfileName());
    GetMesh()->SetCanEverAffectNavigation(Default->GetMesh()->CanEverAffectNavigation());
    GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    GetMesh()->SetRelativeTransform(Default->GetMesh()->GetRelativeTransform());

    // Undo weapon drop (Die)
    auto RestoreWeapon = [&](UPrimitiveComponent* Weapon, const UPrimitiveComponent* DefaultWeapon, FName Socket)
        {
            if (!Weapon || !DefaultWeapon) return;

            Weapon->SetSimulatePhysics(false);
            Weapon->SetCollisionProfileName(DefaultWeapon->GetCollisionProfileName());
            Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Socket);
            Weapon->SetRelativeTransform(DefaultWeapon->GetRelativeTransform());
        };
    RestoreWeapon(RightWeaponMesh, Default->RightWeaponMesh, RightWeaponSocketName);
    RestoreWeapon(LeftWeaponMesh, Default->LeftWeaponMesh, LeftWeaponSocketName);

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    SetActorTransform(InitialTransform, false, nullptr, ETeleportType::ResetPhysics);
    
    CurrentHealth = MaxHealth;
    LimitPoise = MaxPoise;
    CurrentPoise = LimitPoise;
    SetCharacterState(EEnemyState::Idle);

    // A fresh controller restarts BT, blackboard and perception from scratch
    if (AController* OldController = GetController())
    {
        OldController->Destroy();
    }
    SpawnDefaultController();

    NotifyHealthBarUpdate(CurrentHealth, MaxHealth);
}
