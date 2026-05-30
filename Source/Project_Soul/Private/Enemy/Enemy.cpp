#include "Enemy/Enemy.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AEnemy::AEnemy()
{
    PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
    LimitPoise = MaxPoise;
    CurrentPoise = LimitPoise;

	SetCharacterState(EEnemyState::Idle);
    GetCharacterMovement()->MaxAcceleration = 4000.f;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;
    bUseControllerRotationYaw = false;

    AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC) return;
    UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
    if (!BBComp) return;

	BBComp->SetValueAsFloat(FName("MinCombatRange"), MinCombatRange);
	BBComp->SetValueAsFloat(FName("MaxCombatRange"), MaxCombatRange);
}

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
float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, 
    AController* EventInstigator, AActor* DamageCauser)
{
    if (CharacterState == EEnemyState::Dead) return 0.f;
    
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// Apply backstab or critical hit multipliers if applicable, and reset the flags
    if (bBackstabbed)
    {
        if (DamageCauser != AttackInitiator) return 0.f;
        ActualDamage *= BackstabMultiplier;
        bBackstabbed = false;

    }
    else if (bCriticalHit)
    {
		if (DamageCauser != AttackInitiator) return 0.f;
        ActualDamage *= CriticalHitMultiplier;
        bCriticalHit = false;
    }
    
	// Reduce health by the actual damage amount
    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
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

void AEnemy::StartDecisionTimer()
{
    GetWorldTimerManager().SetTimer(DecisionTimerHandle, this, &AEnemy::OnDecisionTimer, 0.5f, false);
}

void AEnemy::OnDecisionTimer()
{
    if (CharacterState == EEnemyState::InCombat) 
    {
        DecideNextAction();
        StartDecisionTimer();
    }
}

// Base decision logic for the enemy's next action
void AEnemy::DecideNextAction()
{
    if (CharacterState != EEnemyState::InCombat) return;
}

// Execute the decided action, ensuring that the enemy is not currently staggered or dead
void AEnemy::PerformAttackPattern(int32 AttackPatternIdx)
{
    if (CharacterState != EEnemyState::InCombat) return;
    if (!Patterns.IsValidIndex(AttackPatternIdx)) return;
    SetCharacterState(EEnemyState::Attack);
    PatternIdx = AttackPatternIdx;
	AttackIdx = 0;

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
    SetCharacterState(EEnemyState::InCombat);
}

// Enable the weapon hitbox for the current attack
void AEnemy::EnableWeaponHitbox()
{
    /*
    UE_LOG(LogTemp, Warning, TEXT("Pattern: %d / Attack: %d / Damage: %.1f"), PatternIdx, AttackIdx,
        BaseDamage * Patterns[PatternIdx].Attacks[AttackIdx].DamageMultiplier);
    */
}

// Disable the weapon hitbox after the attack
void AEnemy::DisableWeaponHitbox()
{
    // Record player dodge vector if the attack was missed
    if ((AlreadyHitActors.Num() <= 0) && Patterns[PatternIdx].Attacks[AttackIdx].MovementSpeed > 0)
    {
        AAIController* AIC = Cast<AAIController>(GetController());
        if (AIC && AIC->GetFocusActor())
        {
            FVector ToPlayer = AIC->GetFocusActor()->GetActorLocation() - GetActorLocation();
            FVector ToPlayerDir = PlayerActionRecord.GetAttackTransform().TransformVector(ToPlayer);
            PlayerActionRecord.RecordDodge(ToPlayerDir);
        }
    }
}

void AEnemy::OnWeaponHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!Patterns.IsValidIndex(PatternIdx)) return;
    FAttackPattern Pattern = Patterns[PatternIdx];
    
    if (!Pattern.Attacks.IsValidIndex(AttackIdx)) return;
    FAttackProperties Attack = Pattern.Attacks[AttackIdx];

    if (!OtherActor || OtherActor == this || AlreadyHitActors.Contains(OtherActor) || Cast<AEnemy>(OtherActor)) return;

    AlreadyHitActors.Add(OtherActor);

    float Multiplier = Attack.DamageMultiplier;

    UGameplayStatics::ApplyDamage(OtherActor, BaseDamage * Multiplier, GetController(), this, nullptr);

    UE_LOG(LogTemp, Warning, TEXT("Hit: %s / Pattern: %d / Attack: %d / Damage: %.1f"),
        *OtherActor->GetName(), PatternIdx, AttackIdx, BaseDamage * Multiplier);
}

// Enable rotation during an attack, adjusting rotation speed based on the attack properties
void AEnemy::EnableAttackRotation()
{
    if (!Patterns.IsValidIndex(PatternIdx)) return;
    if (!Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;
    const FAttackProperties& Attack = Patterns[PatternIdx].Attacks[AttackIdx];

	AAIController* AIC = Cast<AAIController>(GetController());
    if (!AIC || !AIC->GetFocusActor()) return;

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

// Enable movement during an attack, recording forward vector to calculate player dodge direction
void AEnemy::EnableAttackMovement()
{
    if (!Patterns.IsValidIndex(PatternIdx)) return;
    if (!Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;
    const FAttackProperties& Attack = Patterns[PatternIdx].Attacks[AttackIdx];

	if (Attack.MovementSpeed > 0.f)
    {
        // Record attack movement direction vector
        PlayerActionRecord.SetAttackDirection(GetActorForwardVector()); 

		AAIController* AIC = Cast<AAIController>(GetController());
        if (AIC && AIC->GetFocusActor())
        {
            GetCharacterMovement()->MaxWalkSpeed = Attack.MovementSpeed;

            FVector TargetLocation = AIC->GetFocusActor()->GetActorLocation();
            
            // Add predicted player dodge location
            TargetLocation += GetActorTransform().TransformVector(PlayerActionRecord.GetCorrectedOffset());

            AIC->MoveToLocation(TargetLocation);
        }
    }
}

// Disable movement during an attack, setting walk speed to zero
void AEnemy::DisableAttackMovement()
{
    GetCharacterMovement()->MaxWalkSpeed = 0.f;
}

// Handle entering patrol state, called by EnemyAIController when player is lost
void AEnemy::EnterPatrol()
{
    SetCharacterState(EEnemyState::Patrol);

    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
    {
        UFunction* CustomEvent = AnimInst->FindFunction(FName("ExitCombat"));
        if (CustomEvent)
        {
            AnimInst->ProcessEvent(CustomEvent, nullptr);
        }
    }
}

// Handle entering alert state, called by EnemyAIController when player is detected
void AEnemy::EnterAlert()
{
    SetCharacterState(EEnemyState::Alert);
}

// Handle entering combat state, called by EnemyAIController when DetectionLevel is high enough
void AEnemy::EnterCombat()
{
    SetCharacterState(EEnemyState::InCombat);

    if (CombatMontage)
    {
		PlayAnimMontage(CombatMontage);
    }
}

// Handle entering stagger state, playing animation
void AEnemy::EnterStagger()
{
    SetCharacterState(EEnemyState::Stagger);

    if (StaggerMontage)
    {
        PlayAnimMontage(StaggerMontage);
    }
}

// Handle exiting stagger state, resetting poise and returning to combat state
void AEnemy::ExitStagger()
{
    if (CharacterState != EEnemyState::Stagger) return;

    CurrentPoise = LimitPoise;
    SetCharacterState(EEnemyState::InCombat);
}

// Handle entering groggy state, playing animation and providing player opportunity
void AEnemy::EnterGroggy()
{
    SetCharacterState(EEnemyState::Groggy);

    if (GroggyMontage)
    {
        PlayAnimMontage(GroggyMontage);
    }
}

// Handle exiting groggy state, resetting poise and returning to combat state
void AEnemy::ExitGroggy()
{
    if (CharacterState != EEnemyState::Groggy) return;

    LimitPoise = MaxPoise;
    CurrentPoise = LimitPoise;
    SetCharacterState(EEnemyState::InCombat);
}

void AEnemy::GetBackstabbed(AActor* Attacker)
{
    SetCharacterState(EEnemyState::Groggy);
	CurrentPoise = 0.f;
    bBackstabbed = true;
	AttackInitiator = Attacker;

    FVector Direction = GetActorLocation() - Attacker->GetActorLocation();
    Direction.Z = 0.f;
	SetActorRotation(Direction.Rotation());

    if (BackstabMontage)
    {
		PlayAnimMontage(BackstabMontage);
    }
}

void AEnemy::GetCriticalHit(AActor* Attacker)
{
    SetCharacterState(EEnemyState::Groggy);
	CurrentPoise = 0.f;
	bCriticalHit = true;
    AttackInitiator = Attacker;

	FVector Direction = Attacker->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.f;
	SetActorRotation(Direction.Rotation());

    if (CriticalHitMontage)
    {
		PlayAnimMontage(CriticalHitMontage);
    }
}

// Handle death, playing animation
void AEnemy::Die()
{
    SetCharacterState(EEnemyState::Dead);

    if (DeathMontage)
    {
        PlayAnimMontage(DeathMontage);
    }

	AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC)
    {
        AIC->Destroy();
	}
}