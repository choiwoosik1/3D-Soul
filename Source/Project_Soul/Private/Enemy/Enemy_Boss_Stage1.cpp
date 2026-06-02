#include "Enemy/Enemy_Boss_Stage1.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"

// Sets default values
AEnemy_Boss_Stage1::AEnemy_Boss_Stage1()
{
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Create and attach the weapon mesh to the character's hand socket
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), FName("HandGrip_R"));

	WeaponHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponHitbox"));
	WeaponHitbox->SetupAttachment(WeaponMesh);
	WeaponHitbox->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void AEnemy_Boss_Stage1::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the behavior tree and update combat range values in the blackboard
	AAIController* AIC = Cast<AAIController>(GetController());
	AIC->RunBehaviorTree(BTAsset);
	AIC->GetBlackboardComponent()->SetValueAsFloat(FName("MinCombatRange"), MinCombatRange);
	AIC->GetBlackboardComponent()->SetValueAsFloat(FName("MaxCombatRange"), MaxCombatRange);

	// Bind the weapon hitbox overlap event and disable it initially to prevent unintended collisions
	WeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &AEnemy_Boss_Stage1::OnWeaponHitboxOverlap);
	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Start a timer to continuously update the boss's movement speed based on the player's distance
	GetWorldTimerManager().SetTimer(UpdateDistanceTimerHandle, this, &AEnemy_Boss_Stage1::UpdateDistance, 0.1f, true);
}

// Update the boss's movement speed based on the distance to the player and update the blackboard
void AEnemy_Boss_Stage1::UpdateDistance()
{
	if (CharacterState != EEnemyState::InCombat) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	SetSpeedByDistance(Distance);
	Cast<AAIController>(GetController())->GetBlackboardComponent()->SetValueAsFloat(FName("DistanceToTarget"), Distance);
}

// Base decision logic for enemy's next action
void AEnemy_Boss_Stage1::DecideNextAction()
{
	if (CharacterState != EEnemyState::InCombat) return;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	AActor* Target = Cast<AActor>(AIC->GetFocusActor());
	if (!Target) return;

	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	// Find valid attack patterns based on distance to the target
	TArray<int32> ValidAttackPatterns;
	for (int32 i = 0; i < Patterns.Num(); i++)
	{
		if (Distance <= Patterns[i].AttackRange)
			ValidAttackPatterns.Add(i);
	}

	// If there are valid attack patterns, randomly select one and perform the attack
	if (ValidAttackPatterns.Num() > 0)
	{
		int32 SelectedIdx = ValidAttackPatterns[FMath::RandRange(0, ValidAttackPatterns.Num() - 1)];
		PerformAttackPattern(SelectedIdx);
		return;
	}
}

// Handle resuming to combat state, refocusing on the player and updates the blackboard
void AEnemy_Boss_Stage1::ResumeCombat()
{
	Super::ResumeCombat();

	AAIController* AIC = Cast<AAIController>(GetController());
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!AIC || !Player) return;

	AIC->SetFocus(Player);

	if (UBlackboardComponent* BBComp = AIC->GetBlackboardComponent())
	{
		BBComp->SetValueAsObject(FName("TargetActor"), Player);
	}
}

// Enable the weapon hitbox for collision detection during attack animations
void AEnemy_Boss_Stage1::EnableWeaponHitbox()
{
	Super::EnableWeaponHitbox();

	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

// Disable the weapon hitbox to prevent unintended collisions outside of attack animations
void AEnemy_Boss_Stage1::DisableWeaponHitbox()
{
	Super::DisableWeaponHitbox();

	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AlreadyHitActors.Empty();
	AttackIdx++;
}

// Detach the weapon mesh and enable physics simulation on it when the boss dies to create a more dramatic death effect
void AEnemy_Boss_Stage1::Die()
{
	Super::Die();

	if (WeaponMesh)
	{
		WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}