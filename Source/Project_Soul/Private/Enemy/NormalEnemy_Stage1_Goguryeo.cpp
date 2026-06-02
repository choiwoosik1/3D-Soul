#include "Enemy/NormalEnemy_Stage1_Goguryeo.h"
#include "Enemy/Projectile.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "AIController.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ANormalEnemy_Stage1_Goguryeo::ANormalEnemy_Stage1_Goguryeo()
{
	// Create and attach the weapon mesh to the character's hand socket
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), FName("HandGrip_L"));

    WeaponHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponHitbox"));
    WeaponHitbox->SetupAttachment(WeaponMesh);
    WeaponHitbox->SetCollisionObjectType(ECC_WorldDynamic);
    WeaponHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void ANormalEnemy_Stage1_Goguryeo::BeginPlay()
{
    Super::BeginPlay();

    WeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage1_Goguryeo::OnWeaponHitboxOverlap);
	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage1_Goguryeo::DecideNextAction()
{ 
	if (CharacterState != EEnemyState::InCombat) return;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	AActor* Target = Cast<AActor>(AIC->GetFocusActor());
	if (!Target) return;

	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	// If the target is within the melee attack pattern's range, perform that attack immediately
	if (Distance <= Patterns[1].AttackRange)
	{
		PerformAttackPattern(1);
		return;
	}

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
void ANormalEnemy_Stage1_Goguryeo::EnableWeaponHitbox()
{
	Super::EnableWeaponHitbox();

	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

// Disable the weapon hitbox to prevent unintended collisions outside of attack animations
void ANormalEnemy_Stage1_Goguryeo::DisableWeaponHitbox()
{
	Super::DisableWeaponHitbox();

	WeaponHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AlreadyHitActors.Empty();
	AttackIdx++;
}

// Fire a projectile towards the target, calculating the lead based on the target's velocity to increase hit accuracy
void ANormalEnemy_Stage1_Goguryeo::FireProjectile_Implementation()
{
	if (!ProjectileClass) return;

	FVector MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	FRotator FireRotation = WeaponMesh->GetSocketRotation(MuzzleSocketName);

	AAIController* AIC = Cast<AAIController>(GetController());
	if (AIC && AIC->GetFocusActor())
	{
		// Calculate lead for moving targets
		FVector TargetLocation = AIC->GetFocusActor()->GetActorLocation();
		FVector TargetVelocity = AIC->GetFocusActor()->GetVelocity();
		float Distance = FVector::Dist(MuzzleLocation, TargetLocation);
		float TimeToTarget = Distance / ProjectileSpeed;
		FVector PredictedLocation = TargetLocation + TargetVelocity * TimeToTarget / 1.7f;

		DrawDebugSphere(GetWorld(), TargetLocation, 20.f, 12, FColor::Blue, false, 3.f);
		DrawDebugSphere(GetWorld(), PredictedLocation, 20.f, 12, FColor::Yellow, false, 3.f);

		// Record attack location
		PlayerActionRecord.SetAttackDirection(PredictedLocation);

		// Add player dodge prediction
		PredictedLocation += GetActorTransform().TransformVectorNoScale(PlayerActionRecord.GetCorrectedOffset());

		DrawDebugSphere(GetWorld(), PredictedLocation, 20.f, 12, FColor::Green, false, 3.f);
		
		FireRotation = (PredictedLocation - MuzzleLocation).Rotation();
	}

	// Spawn the projectile with the calculated rotation and set its velocity
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AActor* Projectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, FireRotation, SpawnParams);
	
	// Set projectile damage and activate its movement component to propel it forward
	if (Projectile)
	{
		float multiplier = Patterns[PatternIdx].Attacks[AttackIdx++].DamageMultiplier;
		Cast<AProjectile>(Projectile)->SetDamage(BaseDamage* multiplier);

		UProjectileMovementComponent* ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
		if (ProjectileMovement)
		{
			ProjectileMovement->SetVelocityInLocalSpace(FVector::ForwardVector * ProjectileMovement->InitialSpeed);
			ProjectileMovement->Activate();
		}
	}
}

// Handle death logic, detaching the weapon and enabling physics simulation for a more dynamic death effect
void ANormalEnemy_Stage1_Goguryeo::Die()
{
	Super::Die();

	if (WeaponMesh)
	{
		WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}