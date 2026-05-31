#include "Enemy/NormalEnemy_Stage1_Baekje.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "AIController.h"

// Sets default values
ANormalEnemy_Stage1_Baekje::ANormalEnemy_Stage1_Baekje()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), FName("HandGrip_R"));

	WeaponHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponHitbox"));
	WeaponHitbox->SetupAttachment(WeaponMesh);
	WeaponHitbox->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void ANormalEnemy_Stage1_Baekje::BeginPlay()
{
	Super::BeginPlay();

	WeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage1_Baekje::OnWeaponHitboxOverlap);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage1_Baekje::DecideNextAction()
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

// Enable the weapon hitbox for collision detection during attack animations
void ANormalEnemy_Stage1_Baekje::EnableWeaponHitbox()
{
	Super::EnableWeaponHitbox();

	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

// Disable the weapon hitbox to prevent unintended collisions outside of attack animations
void ANormalEnemy_Stage1_Baekje::DisableWeaponHitbox()
{
	Super::DisableWeaponHitbox();

	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AlreadyHitActors.Empty();
	AttackIdx++;
}

void ANormalEnemy_Stage1_Baekje::Die()
{
	Super::Die();

	if (WeaponMesh)
	{
		WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}