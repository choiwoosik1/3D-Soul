#include "Enemy/NormalEnemy.h"
#include "Enemy/EnemyAIController.h"
#include "Enemy/PatrolPath.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ANormalEnemy::ANormalEnemy()
{
    AIControllerClass = AEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// Called when the game starts or when spawned
void ANormalEnemy::BeginPlay()
{
	Super::BeginPlay();
	SetCharacterState(EEnemyState::Patrol);
}

// Get the next patrol point from the patrol path, looping back to the start if necessary
FVector ANormalEnemy::GetNextPatrolPoint()
{
    if (!PatrolPath) return GetActorLocation();
    FVector NextPoint = PatrolPath->GetPatrolPoint(CurrentPatrolIndex);
    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPath->GetPatrolPointCount();
    return NextPoint;
}

// Handle taking damage, transitioning to combat state if necessary, and applying damage logic
float ANormalEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CharacterState == EEnemyState::Dead) return 0.f;

    // If the enemy is not in Combat, transition to InCombat state and set the target actor in the blackboard
    if (CharacterState == EEnemyState::Idle || CharacterState == EEnemyState::Patrol ||
        CharacterState == EEnemyState::Alert || CharacterState == EEnemyState::Groggy)
    {
        AEnemyAIController* AIC = Cast<AEnemyAIController>(GetController());
        if (!AIC) return 0.f;

		AIC->TargetDetected(DamageCauser);
    }

    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

// Handle death logic, including enabling ragdoll physics and destroying the actor after a delay
void ANormalEnemy::Die()
{
    Super::Die();

    // Implement death logic
    float Duration = DeathMontage->GetPlayLength();
    FTimerHandle RagdollTimerHandle;
    GetWorldTimerManager().SetTimer(RagdollTimerHandle, this, &ANormalEnemy::EnableRagdoll, Duration, false);
}

// Enable ragdoll physics on the enemy's mesh, allowing it to react to physics forces and collisions
void ANormalEnemy::EnableRagdoll()
{
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

    FVector GroundLocation = GetCapsuleComponent()->GetComponentLocation();
    GroundLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    GetMesh()->SetWorldLocation(GroundLocation);

    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    GetMesh()->SetAllBodiesSimulatePhysics(true);
    GetMesh()->WakeAllRigidBodies();
    GetMesh()->SetCanEverAffectNavigation(false);
}