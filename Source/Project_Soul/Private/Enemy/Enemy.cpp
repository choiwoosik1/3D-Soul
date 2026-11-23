#include "Enemy/Enemy.h"
#include "Enemy/EnemyDamageTypes.h"
#include "Enemy/PlayerSnapshot.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
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

    // Let the player's spring arm pass through enemies; only world geometry should pull the camera in
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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

// Combat distance the movement logic keeps. Moves by as much as the longest learned range has moved,
// so the enemy holds positions from which its patterns can still start.
float AEnemy::GetMaxCombatRange() const
{
    float OriginalReach = 0.f;
    float LearnedReach = 0.f;
    for (int32 i = 0; i < Patterns.Num(); i++)
    {
        OriginalReach = FMath::Max(OriginalReach, Patterns[i].AttackRange);
        LearnedReach = FMath::Max(LearnedReach, GetEffectiveRange(i));
    }
    return FMath::Clamp(MaxCombatRange + LearnedReach - OriginalReach,
        MinCombatRange, MaxCombatRange * MaxCombatRangeLimitScale);
}

// Push the current combat range to the blackboard. Works with any AI controller that runs a behavior tree.
void AEnemy::UpdateBlackboardCombatRange()
{
    AAIController* AIC = Cast<AAIController>(GetController());
    UBlackboardComponent* BB = AIC ? AIC->GetBlackboardComponent() : nullptr;
    if (!BB) return;

    BB->SetValueAsFloat(TEXT("MinCombatRange"), GetMinCombatRange());
    BB->SetValueAsFloat(TEXT("MaxCombatRange"), GetMaxCombatRange());
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
	float MaxRange = GetMaxCombatRange();

	// If the distance is less than or equal to the maximum combat range, set the speed to StrafeSpeed
    if (Distance <= MaxRange)
    {
        GetCharacterMovement()->MaxWalkSpeed = StrafeSpeed;
    }
	// If the distance is between 1.0 and 1.2 times the maximum combat range, interpolate the speed between StrafeSpeed and WalkSpeed
	else if (Distance <= 1.2f * MaxRange)
    {
        GetCharacterMovement()->MaxWalkSpeed = FMath::GetMappedRangeValueClamped(
            FVector2D(MaxRange, 1.2f * MaxRange), FVector2D(StrafeSpeed, WalkSpeed), Distance);
    }
	// If the distance is between 1.2 and 1.5 times the maximum combat range, interpolate the speed between WalkSpeed and RunSpeed
	else if (Distance <= 1.5f * MaxRange)
    {
        GetCharacterMovement()->MaxWalkSpeed = FMath::GetMappedRangeValueClamped(
            FVector2D(1.2f * MaxRange, 1.5f * MaxRange), FVector2D(WalkSpeed, RunSpeed), Distance);
    }
	// If the distance is greater than 1.5 times the maximum combat range, set the speed to RunSpeed
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
    }
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
    bCriticalHit = false;
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
    if (GetWorld()->GetTimeSeconds() - AttackStartTime >= BehaviorStats::PunishIgnoreTime)
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
        bIsInvincible = false;
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

    // An attack or a roll counts as started on the first decision that sees it
    const FPlayerSnapshot Snapshot = FPlayerSnapshot::Capture(Target);
    const bool bTargetAttacking = Snapshot.Action == EPlayerAction::Attacking;
    const bool bTargetRolling = Snapshot.Action == EPlayerAction::Rolling;
    const bool bAttackStarted = bTargetAttacking && !PlayerActionRecord.bTargetWasAttacking;
    const bool bRollStarted = bTargetRolling && !PlayerActionRecord.bTargetWasRolling;
    PlayerActionRecord.bTargetWasAttacking = bTargetAttacking;
    PlayerActionRecord.bTargetWasRolling = bTargetRolling;

    // A counterattack right after a pattern counts against it even if it falls short
    if (bAttackStarted && PunishWatchIdx != INDEX_NONE)
    {
        ClosePunishWatch(true);
    }

    FWaitObservation& Wait = PlayerActionRecord.Wait;
    if (Wait.bActive)
    {
        Wait.bAttackedFirst |= bTargetAttacking;
        Wait.bRolled |= bTargetRolling;
    }

    // A roll catch is waiting for its moment; nothing else is decided until it starts
    if (PendingCatchIdx != INDEX_NONE) return;

    // Punish: answer the target's attack. Rolled once per attack.
    if (bAttackStarted && Now >= NextPunishTime && FMath::FRand() < GetReactionChance(EReactionAttack::Punish))
    {
        if (StartReactionAttack(EReactionAttack::Punish, Distance)) return;
    }

    // Roll catch: judged against where the roll is heading. Rolled once per roll.
    if (bRollStarted && Now >= NextRollCatchTime && FMath::FRand() < GetReactionChance(EReactionAttack::RollCatch))
    {
        const FVector Landing = Snapshot.Location + FVector(Snapshot.Velocity.X, Snapshot.Velocity.Y, 0.f) * RollLookAhead;
        if (StartReactionAttack(EReactionAttack::RollCatch, FVector::Dist(GetActorLocation(), Landing))) return;
    }

    // Leave a pause after the previous pattern
    if (Now - LastAttackEndTime < MinAttackGap) return;

    const int32 SelectedIdx = PickPattern(Distance);

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
        Wait.StartTime = Now;
        Wait.EndTime = Now + FMath::FRandRange(WaitDuration.Min, WaitDuration.Max);
        // UE_LOG(LogTemp, Warning, TEXT("WaitStart Chance=%.2f"), GetWaitChance());
        return;
    }

    if (SelectedIdx != INDEX_NONE)
    {
        PerformAttackPattern(SelectedIdx);
    }
}

// Actor this enemy is currently fighting
AActor* AEnemy::GetCombatTarget() const
{
    const AAIController* AIC = Cast<AAIController>(GetController());
    return AIC ? AIC->GetFocusActor() : nullptr;
}

// Chance to use a reaction attack.
// Punish follows how often past punishes landed. Roll catch follows how often the player rolls while not attacked,
// scaled down when catches stop landing.
float AEnemy::GetReactionChance(EReactionAttack Type) const
{
    constexpr float MinChance = 0.2f;
    constexpr float MaxChance = 0.9f;

    const FPlayerBehaviorStats& Behavior = PlayerActionRecord.Behavior;
    if (Type == EReactionAttack::RollCatch)
    {
        const float Habit = FMath::Clamp(Behavior.RollHabit.Get(BehaviorStats::RollHabitPrior), MinChance, MaxChance);
        return Habit * Behavior.RollCatch.Get(BehaviorStats::RollCatchHitPrior);
    }
    return FMath::Clamp(Behavior.Punish.Get(BehaviorStats::PunishHitPrior), MinChance, MaxChance);
}

// Start a reaction attack judged against the given distance. A roll catch is held back
// so its first hitbox opens as the roll ends. Returns false when no pattern fits.
bool AEnemy::StartReactionAttack(EReactionAttack Type, float Distance)
{
    const int32 Idx = PickPattern(Distance, Type);
    if (Idx == INDEX_NONE) return false;

    float Delay = 0.f;
    if (Type == EReactionAttack::RollCatch)
    {
        // Faster patterns are held back so the first hitbox opens as the roll ends; slower ones start at once.
        // A pattern not measured yet also starts at once.
        const float Startup = Patterns[Idx].OutcomeStats.StartupTime;
        if (Startup > 0.f)
        {
            Delay = RollCatchLandTime - DecisionInterval * 0.5f - Startup;
        }
    }

    // UE_LOG(LogTemp, Warning, TEXT("Reaction %s P=%d Chance=%.2f Distance=%.0f Delay=%.2f"),
    //    LexToString(Type), Idx, GetReactionChance(Type), Distance, FMath::Max(Delay, 0.f));

    if (Delay > 0.f)
    {
        PendingCatchIdx = Idx;
        PendingCatchDistance = Distance;
        GetWorldTimerManager().SetTimer(RollCatchTimerHandle, this, &AEnemy::StartPendingCatch, Delay, false);
        return true;
    }

    BeginReactionAttack(Idx, Type, Distance);
    return true;
}

// Pick one attack pattern that can reach the target, with a chance proportional to its weight.
// A normal decision weighs patterns by ScorePattern; a reaction keeps only the patterns that can arrive in time
// and weighs them by how often each one has worked as that reaction. Returns INDEX_NONE when no pattern qualifies.
int32 AEnemy::PickPattern(float Distance, EReactionAttack Reaction) const
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

    // Time left until a roll catch should land; the roll began somewhere within the last decision interval
    const float CatchRemaining = RollCatchLandTime - DecisionInterval * 0.5f;

    // Patterns that do not qualify keep a weight of zero
    TArray<float> Weights;
    Weights.Init(0.f, Patterns.Num());
    float TotalWeight = 0.f;
    for (int32 i = 0; i < Patterns.Num(); i++)
    {
        if (Distance > GetEffectiveRange(i)) continue;

        const FPatternOutcomeStats& Stats = Patterns[i].OutcomeStats;
        const float Startup = Stats.StartupTime;
        switch (Reaction)
        {
        case EReactionAttack::Punish:
            // Only the fastest patterns can answer an attack in time
            if (Startup <= 0.f || Startup > FastestStartup + PunishStartupMargin) continue;
            Weights[i] = Stats.PunishHit.Get(BehaviorStats::PunishHitPrior);
            break;

        case EReactionAttack::RollCatch:
            // Patterns are chosen for catching by hand; StartReactionAttack delays the start to match the roll
            if (!Patterns[i].bAllowRollCatch) continue;
            Weights[i] = Stats.CatchHit.Get(BehaviorStats::RollCatchHitPrior);
            break;

        default:
            Weights[i] = ScorePattern(i);
            break;
        }

        // Reactions avoid the pattern used last as well, so the same answer does not come every time
        if (Reaction != EReactionAttack::None && i == PatternIdx)
        {
            Weights[i] *= RepeatPenalty;
        }

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

// Attack range after learning. Can be shorter or longer than AttackRange.
float AEnemy::GetEffectiveRange(int32 Idx) const
{
    return Patterns[Idx].AttackRange * Patterns[Idx].OutcomeStats.RangeScale;
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

// Chance to hold an attack. Equals BaseWaitChance with no data, and drops toward zero
// against a player who keeps attacking first while the enemy waits.
float AEnemy::GetWaitChance() const
{
    const float Aggression = PlayerActionRecord.Behavior.Aggression.Get(BehaviorStats::AggressionPrior);
    return BaseWaitChance * (1.f - Aggression) / (1.f - BehaviorStats::AggressionPrior);
}

// Show the learned statistics above the enemy until the next decision
void AEnemy::DrawBehaviorDebug()
{
    if (!bShowBehaviorDebug) return;

    const FPlayerBehaviorStats& Behavior = PlayerActionRecord.Behavior;
    FString Text = FString::Printf(TEXT("Guard %.0f%%  Aggression %.0f%%  Roll %.0f%%  Wait %.0f%%  Punish %.0f%%  Catch %.0f%%  CombatRange %.0f%s"),
        Behavior.Guard.Get(BehaviorStats::GuardPrior) * 100.f,
        Behavior.Aggression.Get(BehaviorStats::AggressionPrior) * 100.f,
        Behavior.RollHabit.Get(BehaviorStats::RollHabitPrior) * 100.f,
        GetWaitChance() * 100.f,
        GetReactionChance(EReactionAttack::Punish) * 100.f,
        GetReactionChance(EReactionAttack::RollCatch) * 100.f,
        GetMaxCombatRange(),
        PlayerActionRecord.Wait.bActive ? TEXT("  [Waiting]") : TEXT(""));

    for (int32 i = 0; i < Patterns.Num(); i++)
    {
        const FPatternOutcomeStats& Stats = Patterns[i].OutcomeStats;
        Text += FString::Printf(TEXT("\nP%d  W%.2f  H%.1f D%.1f G%.1f O%.1f  Hit %.0f%%  Punished %.0f%%  PunishHit %.0f%%  CatchHit %.0f%%  Range %.0f (x%.2f)  Startup %.2fs"),
            i, ScorePattern(i), Stats.Hit, Stats.Dodged, Stats.Guarded, Stats.OutRanged,
            Stats.GetHitRate(BehaviorStats::HitPrior) * 100.f,
            Stats.Punished.Get(BehaviorStats::PunishedPrior) * 100.f,
            Stats.PunishHit.Get(BehaviorStats::PunishHitPrior) * 100.f,
            Stats.CatchHit.Get(BehaviorStats::RollCatchHitPrior) * 100.f,
            GetEffectiveRange(i), Stats.RangeScale, Stats.StartupTime);
    }

    const float Duration = GetWorldTimerManager().GetTimerRate(DecisionTimerHandle);
    DrawDebugString(GetWorld(), FVector(0.f, 0.f, 150.f), Text, this, FColor::White, Duration, true);
}

// Timer callback: the held-back roll catch starts now
void AEnemy::StartPendingCatch()
{
    const int32 Idx = PendingCatchIdx;
    PendingCatchIdx = INDEX_NONE;

    if (CharacterState != EEnemyState::InCombat || !Patterns.IsValidIndex(Idx)) return;
    BeginReactionAttack(Idx, EReactionAttack::RollCatch, PendingCatchDistance);
}

// Start the chosen pattern as a reaction attack
void AEnemy::BeginReactionAttack(int32 Idx, EReactionAttack Type, float Distance)
{
    PerformAttackPattern(Idx);
    PlayerActionRecord.ReactionAttack = Type;

    // The cooldown starts when this pattern ends, so a long pattern does not use up its own cooldown
    CooldownReaction = Type;

    if (Type == EReactionAttack::RollCatch)
    {
        // The roll that triggered the catch is not a dodge of it
        PlayerActionRecord.Swing.bRollHandled = true;

        // Range learning should judge the pattern against where the target was heading
        PlayerActionRecord.Run.StartDistance = Distance;

        // The roll that started the catch was seen during the wait; the wait ends here, too early to judge holding back
        CloseWait(true);
    }
}

// Execute the decided attack pattern
void AEnemy::PerformAttackPattern(int32 AttackPatternIdx)
{
    if (CharacterState != EEnemyState::InCombat) return;
    if (!Patterns.IsValidIndex(AttackPatternIdx)) return;

	// Reset the reaction attack type, as this is a new pattern
    PlayerActionRecord.ReactionAttack = EReactionAttack::None;

    // Decisions stop while attacking, so the next one must judge the target's action afresh
    PlayerActionRecord.bTargetWasAttacking = false;
    PlayerActionRecord.bTargetAttackingInPattern = false;
    PlayerActionRecord.bTargetWasRolling = false;

    // From here until shortly after the pattern ends, being hit counts against this pattern
    ClosePunishWatch(false);
    PunishWatchIdx = AttackPatternIdx;

    SetCharacterState(EEnemyState::Attack);
    PatternIdx = AttackPatternIdx;
    AttackIdx = 0;

    // Remember where this pattern started from, for range learning
    PlayerActionRecord.Run = FPatternRun();
    PlayerActionRecord.Run.StartDistance = GetDistanceTo(GetCombatTarget());

    // A new combo starts without a previous dodge
    PlayerActionRecord.PrevDodgeBucket = INDEX_NONE;
    PlayerActionRecord.PendingDodgeSample.bActive = false;

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

// Runs every 0.1 seconds during an attack pattern and notes what the player does between decisions
void AEnemy::ObserveSwing()
{
    if (CharacterState != EEnemyState::Attack)
    {
        GetWorldTimerManager().ClearTimer(SwingObserveTimerHandle);
        return;
    }

    // A target that was invincible while inside the hitbox gets no new overlap event, so check again
    AActor* Target = GetCombatTarget();
    if (Target && RightWeaponHitbox->GetCollisionEnabled() != ECollisionEnabled::NoCollision
        && (RightWeaponHitbox->IsOverlappingActor(Target) || LeftWeaponHitbox->IsOverlappingActor(Target)))
    {
        TryWeaponHit(Target);
    }

    const EPlayerAction Action = FPlayerSnapshot::Capture(Target).Action;

    // A counterattack begun during the pattern counts against it even if it falls short
    const bool bTargetAttacking = Action == EPlayerAction::Attacking;
    if (bTargetAttacking && !PlayerActionRecord.bTargetAttackingInPattern
        && GetWorld()->GetTimeSeconds() - AttackStartTime >= BehaviorStats::PunishIgnoreTime)
    {
        ClosePunishWatch(true);
    }
    PlayerActionRecord.bTargetAttackingInPattern = bTargetAttacking;

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
        Swing.bRolled |= Patterns.IsValidIndex(PatternIdx) && GetDistanceTo(Target) <= GetEffectiveRange(PatternIdx);
    }
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

    // A roll catch starts after the dodge already happened: aim straight at the target, and keep it out of dodge learning
    if (PlayerActionRecord.ReactionAttack == EReactionAttack::RollCatch)
    {
        GetCharacterMovement()->MaxWalkSpeed = Attack.MovementSpeed;
        AIC->MoveToLocation(Target->GetActorLocation(), MinCombatRange, true, true, true);
        return;
    }

    const FVector TargetLocation = Target->GetActorLocation();
    FAttackSample& Sample = PlayerActionRecord.PendingDodgeSample;

    // The sample and the correction are decided once, on the first call of the swing
    const bool bReAim = Sample.bActive && Sample.PatternIdx == PatternIdx && Sample.AttackIdx == AttackIdx;
    if (!bReAim)
    {
        // Attack axis: horizontal direction from this enemy to the target at commit time
        const FVector Forward = (TargetLocation - GetActorLocation()).GetSafeNormal2D();

        // Predict from how the target dodged the previous lunge of this combo
        Sample.Begin(PatternIdx, AttackIdx, Forward, TargetLocation, PlayerActionRecord.PrevDodgeBucket);
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
    AIC->MoveToLocation(AimLocation, CloseToTarget, true, true, true);
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
    if (FMath::FRand() > Confidence * MaxAimShiftChance) return BaseLocation;

    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
    const FVector Shift = Forward * Offset.X + Right * Offset.Y;
    return BaseLocation + Shift.GetClampedToMaxSize(MaxAimShift);
}

// Disable movement during an attack, setting walk speed to zero
void AEnemy::DisableAttackMovement()
{
    GetCharacterMovement()->MaxWalkSpeed = 0.f;
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

// Apply the current swing to an actor touching the weapon hitbox.
// An invincible target is noted but not marked as hit, so the same swing can still land once invincibility ends.
void AEnemy::TryWeaponHit(AActor* OtherActor)
{
    if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;
    const FAttackProperties& Attack = Patterns[PatternIdx].Attacks[AttackIdx];

    if (!OtherActor || Cast<AEnemy>(OtherActor) || AlreadyHitActors.Contains(OtherActor)) return;

    const bool bIsTarget = OtherActor == GetCombatTarget();
    const FPlayerSnapshot Snapshot = FPlayerSnapshot::Capture(OtherActor);
    FSwingObservation& Swing = PlayerActionRecord.Swing;

    if (Snapshot.bInvincible)
    {
        // Rolled through the swing. Invincibility from anything else says nothing about this swing, so drop it.
        if (bIsTarget && Swing.bOpen && !Swing.bReached)
        {
            Swing.bReached = true;
            Swing.ContactOutcome = EAttackOutcome::Dodged;
            Swing.bOpen = Snapshot.Action == EPlayerAction::Rolling;
        }
        return;
    }

    AlreadyHitActors.Add(OtherActor);

    // Judge from the state at contact, before the damage changes it. A real hit replaces an earlier rolled-through touch.
    if (bIsTarget && Swing.bOpen)
    {
        Swing.bReached = true;
        Swing.bGuarded = Snapshot.Action == EPlayerAction::Guarding;
        Swing.ContactOutcome = Swing.bGuarded && Attack.GuardResponse == EGuardResponse::Block
            ? EAttackOutcome::Guarded : EAttackOutcome::Hit;
    }

    // Determine the guard response type based on the attack's guard response property
    TSubclassOf<UDamageType> GuardResponseType;
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

    const float Damage = BaseDamage * Attack.DamageMultiplier;
    UGameplayStatics::ApplyDamage(OtherActor, Damage, GetController(), this, GuardResponseType);
}

// Disable the weapon hitbox after the attack ends, and learn where the target went if a lunge was tracked
void AEnemy::DisableWeaponHitbox()
{
    if (RightWeaponHitbox->GetCollisionEnabled() == ECollisionEnabled::NoCollision) return;

    RightWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftWeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;

    // Resolve the pending lunge sample: where did the target end up compared to where it stood at commit time?
    FAttackSample& Sample = PlayerActionRecord.PendingDodgeSample;
    AAIController* AIC = Cast<AAIController>(GetController());
    if (Sample.bActive && AIC && AIC->GetFocusActor())
    {
        Sample.ResolvedLocation = AIC->GetFocusActor()->GetActorLocation();

        // Log the result of the lunge attack, including pattern index, attack index, and whether any actors were hit
        // UE_LOG(LogTemp, Warning, TEXT("  P=%d A=%d Hit=%d"), PatternIdx, AttackIdx, AlreadyHitActors.Num() > 0);

        // The next lunge of this combo is predicted from this dodge
        PlayerActionRecord.PrevDodgeBucket = RecordSample(Sample);
    }
    Sample.bActive = false;

    // Record how the player dealt with this swing
    FSwingObservation& Swing = PlayerActionRecord.Swing;
    if (Swing.bOpen)
    {
        const EAttackOutcome Outcome = Swing.Resolve();

        // Collect the swing into the pattern-level result used for range learning
        FPatternRun& Run = PlayerActionRecord.Run;
        Run.bResolved = true;
        Run.bReached |= Swing.bReached;
        Run.bAllOutRanged &= Outcome == EAttackOutcome::OutRanged;

        // Swing outcome and guard habit, shared with same-class enemies
        const int32 Idx = PatternIdx;
        const bool bGuarded = Swing.bGuarded;
        ShareLearning([Idx, Outcome, bGuarded](AEnemy& Enemy, float Weight)
            {
                Enemy.Patterns[Idx].OutcomeStats.Record(Outcome, Weight);

                // Guard habit only counts swings the player had to deal with
                if (Outcome != EAttackOutcome::OutRanged)
                {
                    Enemy.PlayerActionRecord.Behavior.Guard.Record(bGuarded, Weight);
                }
            });

        // The first swing decides whether a punish attempt worked
        CloseReactionAttempt(Outcome == EAttackOutcome::Hit);

        // UE_LOG(LogTemp, Warning, TEXT("SwingOutcome P=%d A=%d %s Reached=%d Rolled=%d Carried=%d"),
        //     PatternIdx, AttackIdx, LexToString(Outcome), Swing.bReached, Swing.bRolled, Swing.bRollHandled);
    }

    // The next swing of the pattern starts with a clean observation; a roll still in progress belongs to this swing
    Swing = FSwingObservation();
    Swing.bRollHandled = FPlayerSnapshot::Capture(GetCombatTarget()).Action == EPlayerAction::Rolling;

    AlreadyHitActors.Empty();
    AttackIdx++;
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

    ShareLearning([&Sample, &Offset](AEnemy& Enemy, float Weight)
        {
            TArray<FAttackProperties>& Attacks = Enemy.Patterns[Sample.PatternIdx].Attacks;
            if (!Attacks.IsValidIndex(Sample.AttackIdx)) return;

            Attacks[Sample.AttackIdx].DodgeStats.Record(Offset, Sample.PrevBucket, Weight);
            Enemy.PlayerActionRecord.DodgeFallback.Record(Offset, Sample.PrevBucket, Weight);
        });

    return static_cast<int32>(FDodgeStats::Classify(Offset));
}

// Interrupt the current attack, disable the weapon hitbox, and prevent parrying
void AEnemy::InterruptCurrentAttack()
{
    // Discard the pending dodge sample so an interrupted attack is never learned as a dodge
    PlayerActionRecord.PendingDodgeSample.bActive = false;
    PlayerActionRecord.PrevDodgeBucket = INDEX_NONE;

    // A pattern cut short only tells something about range if it had already reached the target
    if (PlayerActionRecord.Swing.bReached)
    {
        PlayerActionRecord.Run.bReached = true;
    }
    if (PlayerActionRecord.Run.bReached)
    {
        UpdateRangeScale(PatternIdx);
    }
    PlayerActionRecord.Run = FPatternRun();

    // Discard the swing being judged so an interrupted swing is never recorded
    PlayerActionRecord.Swing = FSwingObservation();

    // A hit that counts as a punish was already recorded when the damage arrived.
    // Anything still open belongs to a pattern that never played out, so it is dropped without a record.
    PunishWatchIdx = INDEX_NONE;
    GetWorldTimerManager().ClearTimer(PunishWatchTimerHandle);

    // Drop a roll catch that has not started yet
    PendingCatchIdx = INDEX_NONE;
    GetWorldTimerManager().ClearTimer(RollCatchTimerHandle);

    // A punish stopped before its first swing resolved has failed
    CloseReactionAttempt(false);
    StartReactionCooldown();

    CloseWait();
    PlayerActionRecord.bTargetWasAttacking = false;
    PlayerActionRecord.bTargetWasRolling = false;

    bCanBeParried = false;
    DisableWeaponHitbox();
}

// Reset the enemy's state to InCombat after completing an attack pattern
void AEnemy::FinishAttackPattern()
{
    if (CharacterState != EEnemyState::Attack) return;

    // Patterns without a weapon hitbox never reach the point where a wait is normally closed
    CloseWait();
    PlayerActionRecord.ReactionAttack = EReactionAttack::None;

    StartReactionCooldown();

    // Keep the punish watch open for a moment after the pattern, then record that nothing happened
    constexpr float PunishGrace = 0.3f;
    GetWorldTimerManager().SetTimer(PunishWatchTimerHandle,
        FTimerDelegate::CreateUObject(this, &AEnemy::ClosePunishWatch, false), PunishGrace, false);

    UpdateRangeScale(PatternIdx);
    LastAttackEndTime = GetWorld()->GetTimeSeconds();
    ResumeCombat();
}

// Start the cooldown of the reaction attack that just ended, counted from the end of its pattern
void AEnemy::StartReactionCooldown()
{
    if (CooldownReaction == EReactionAttack::None) return;

    const bool bCatch = CooldownReaction == EReactionAttack::RollCatch;
    const FFloatInterval& Cooldown = bCatch ? RollCatchCooldown : PunishCooldown;
    float& NextTime = bCatch ? NextRollCatchTime : NextPunishTime;
    NextTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(Cooldown.Min, Cooldown.Max);

    CooldownReaction = EReactionAttack::None;
}

// End the wait in progress and record what the player did during it.
// Doing nothing through a short wait says little, so it counts in proportion to how long the player was watched.
// bCutShort: the enemy ended the wait early, so not attacking is not counted as holding back.
void AEnemy::CloseWait(bool bCutShort)
{
    constexpr float FullWeightTime = 1.5f;

    FWaitObservation& Wait = PlayerActionRecord.Wait;
    if (!Wait.bActive) return;

    const bool bAttackedFirst = Wait.bAttackedFirst;
    const bool bRolled = Wait.bRolled;
    const float Watched = GetWorld()->GetTimeSeconds() - Wait.StartTime;
    const float QuietWeight = FMath::Clamp(Watched / FullWeightTime, 0.f, 1.f);
    const float AggressionWeight = bAttackedFirst ? 1.f : (bCutShort ? 0.f : QuietWeight);
    const float RollWeight = bRolled ? 1.f : QuietWeight;

    // A zero weight records nothing
    ShareLearning([bAttackedFirst, bRolled, AggressionWeight, RollWeight](AEnemy& Enemy, float Weight)
        {
            FPlayerBehaviorStats& Behavior = Enemy.PlayerActionRecord.Behavior;
            Behavior.Aggression.Record(bAttackedFirst, Weight * AggressionWeight);
            Behavior.RollHabit.Record(bRolled, Weight * RollWeight);
        });
    // UE_LOG(LogTemp, Warning, TEXT("WaitResult AttackedFirst=%d Rolled=%d Watched=%.2f"), bAttackedFirst, bRolled, Watched);

    Wait = FWaitObservation();
}

// Record whether a reaction attack landed its first swing, for the enemy and for the pattern that was used
void AEnemy::CloseReactionAttempt(bool bHit)
{
    const EReactionAttack Type = PlayerActionRecord.ReactionAttack;
    if (Type == EReactionAttack::None) return;
    PlayerActionRecord.ReactionAttack = EReactionAttack::None;

    const int32 Idx = PatternIdx;
    ShareLearning([Type, bHit, Idx](AEnemy& Enemy, float Weight)
        {
            const bool bCatch = Type == EReactionAttack::RollCatch;

            FPlayerBehaviorStats& Behavior = Enemy.PlayerActionRecord.Behavior;
            (bCatch ? Behavior.RollCatch : Behavior.Punish).Record(bHit, Weight);

            if (Enemy.Patterns.IsValidIndex(Idx))
            {
                FPatternOutcomeStats& Stats = Enemy.Patterns[Idx].OutcomeStats;
                (bCatch ? Stats.CatchHit : Stats.PunishHit).Record(bHit, Weight);
            }
        });
    // UE_LOG(LogTemp, Warning, TEXT("ReactionResult %s P=%d Hit=%d"), LexToString(Type), Idx, bHit);
}


// End the punish watch and record whether the player hit the enemy during the watched pattern
void AEnemy::ClosePunishWatch(bool bPunished)
{
    // Always drop the grace timer so it can never close the watch of a later pattern
    GetWorldTimerManager().ClearTimer(PunishWatchTimerHandle);

    if (!Patterns.IsValidIndex(PunishWatchIdx)) return;

    const int32 Idx = PunishWatchIdx;
    PunishWatchIdx = INDEX_NONE;

    ShareLearning([Idx, bPunished](AEnemy& Enemy, float Weight)
        {
            Enemy.Patterns[Idx].OutcomeStats.Punished.Record(bPunished, Weight);
        });
    // UE_LOG(LogTemp, Warning, TEXT("PunishedResult P=%d Punished=%d"), Idx, bPunished);
}

// Learn from the pattern that just ended how far it can be started from, then clear the record.
// Reaching the target from near the edge of the range pushes the range out; missing entirely pulls it in.
void AEnemy::UpdateRangeScale(int32 Idx)
{
    constexpr float GrowStep = 0.03f;
    constexpr float ShrinkStep = 0.05f;
    constexpr float EdgeBand = 0.15f;
    constexpr float MinScaleFloor = 0.7f;
    constexpr float CombatRangeMargin = 50.f;

    const FPatternRun Run = PlayerActionRecord.Run;
    PlayerActionRecord.Run = FPatternRun();
    if (!Patterns.IsValidIndex(Idx)) return;

    const FAttackPattern& Pattern = Patterns[Idx];
    const float OldScale = Pattern.OutcomeStats.RangeScale;

    // A pattern that moves the enemy during the attack can reach much farther than one that does not
    const bool bMoves = Pattern.Attacks.ContainsByPredicate([](const FAttackProperties& Attack)
        {
            return Attack.MovementSpeed > 0.f;
        });
    const float MaxScale = bMoves ? MaxRangeScaleMoving : MaxRangeScaleStatic;

    // Never shrink past the distance the enemy is allowed to stand at, or the pattern could never start
    const float MinScale = FMath::Min(1.f,
        FMath::Max(MinScaleFloor, (MinCombatRange + CombatRangeMargin) / FMath::Max(Pattern.AttackRange, 1.f)));

    float NewScale = OldScale;
    if (Run.bReached && Run.StartDistance >= GetEffectiveRange(Idx) * (1.f - EdgeBand))
    {
        NewScale = FMath::Min(OldScale + GrowStep, MaxScale);
    }
    else if (Run.bResolved && Run.bAllOutRanged)
    {
        NewScale = FMath::Max(OldScale - ShrinkStep, MinScale);
    }

    const float Delta = NewScale - OldScale;
    if (Delta == 0.f) return;

    // UE_LOG(LogTemp, Warning, TEXT("RangeScale P=%d Start=%.0f Reached=%d %.2f -> %.2f"),
    //    Idx, Run.StartDistance, Run.bReached, OldScale, NewScale);

    // Peers take part of the same change; the bounds are shared because the patterns are the same
    ShareLearning([Idx, Delta, MinScale, MaxScale](AEnemy& Enemy, float Weight)
        {
            float& Scale = Enemy.Patterns[Idx].OutcomeStats.RangeScale;
            Scale = FMath::Clamp(Scale + Delta * Weight, MinScale, MaxScale);
            Enemy.UpdateBlackboardCombatRange();
        });
}

// Apply one record to this enemy at full weight, and to every peer at the share ratio
void AEnemy::ShareLearning(TFunctionRef<void(AEnemy& Enemy, float Weight)> Apply)
{
    Apply(*this, 1.f);

    if (LearningShareRatio <= 0.f) return;
    for (const TWeakObjectPtr<AEnemy>& Peer : GetPeers())
    {
        if (Peer.IsValid())
        {
            Apply(*Peer, LearningShareRatio);
        }
    }
}

// Enemies of exactly this class with the same pattern layout, found once on first use.
// Enemies are never destroyed (they respawn), so the list does not go stale.
const TArray<TWeakObjectPtr<AEnemy>>& AEnemy::GetPeers()
{
    if (!bPeersCached)
    {
        bPeersCached = true;
        for (TActorIterator<AEnemy> It(GetWorld()); It; ++It)
        {
            AEnemy* Other = *It;
            if (Other != this && Other->GetClass() == GetClass() && Other->Patterns.Num() == Patterns.Num())
            {
                Peers.Add(Other);
            }
        }
    }
    return Peers;
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
        RightWeaponMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
        RightWeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        RightWeaponMesh->SetSimulatePhysics(true);
    }

	if (LeftWeaponMesh)
	{
		LeftWeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        LeftWeaponMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		LeftWeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		LeftWeaponMesh->SetSimulatePhysics(true);
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
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetMesh()->SetSimulatePhysics(false);
    GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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

// Drop observations left over from a previous fight. Learned statistics are kept.
void AEnemy::ResetDecisionState()
{
    PlayerActionRecord.Run = FPatternRun();
    PlayerActionRecord.Swing = FSwingObservation();
    PlayerActionRecord.Wait = FWaitObservation();
    PlayerActionRecord.ReactionAttack = EReactionAttack::None;
    PlayerActionRecord.bTargetWasAttacking = false;
    PlayerActionRecord.bTargetWasRolling = false;
	PlayerActionRecord.PrevDodgeBucket = INDEX_NONE;
	PlayerActionRecord.PendingDodgeSample.bActive = false;

    CooldownReaction = EReactionAttack::None;
    NextPunishTime = 0.f;
    PunishWatchIdx = INDEX_NONE;
    GetWorldTimerManager().ClearTimer(PunishWatchTimerHandle);
    NextRollCatchTime = 0.f;
    PendingCatchIdx = INDEX_NONE;
    GetWorldTimerManager().ClearTimer(RollCatchTimerHandle);
}
