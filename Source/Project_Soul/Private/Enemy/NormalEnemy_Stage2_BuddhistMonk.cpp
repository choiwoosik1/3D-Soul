#include "Enemy/NormalEnemy_Stage2_BuddhistMonk.h"
#include "Enemy/Projectile.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "AIController.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ANormalEnemy_Stage2_BuddhistMonk::ANormalEnemy_Stage2_BuddhistMonk()
{
	RightFootHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightFootHitbox"));
	LeftFootHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftFootHitbox"));
}

// Called after the actor's components have been initialized
void ANormalEnemy_Stage2_BuddhistMonk::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	RightFootHitbox->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightFootSocketName);
	RightFootHitbox->SetCollisionObjectType(ECC_WorldDynamic);
	RightFootHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	RightFootHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RightFootHitbox->SetGenerateOverlapEvents(true);
	RightFootHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightFootHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage2_BuddhistMonk::OnWeaponHitboxOverlap);

	LeftFootHitbox->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, LeftFootSocketName);
	LeftFootHitbox->SetCollisionObjectType(ECC_WorldDynamic);
	LeftFootHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	LeftFootHitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	LeftFootHitbox->SetGenerateOverlapEvents(true);
	LeftFootHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftFootHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage2_BuddhistMonk::OnWeaponHitboxOverlap);

	RightWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage2_BuddhistMonk::OnWeaponHitboxOverlap);
	LeftWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage2_BuddhistMonk::OnWeaponHitboxOverlap);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage2_BuddhistMonk::DecideNextAction()
{
	Super::DecideNextAction();
}

void ANormalEnemy_Stage2_BuddhistMonk::EnableWeaponHitbox()
{
	Super::EnableWeaponHitbox();

	RightFootHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LeftFootHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ANormalEnemy_Stage2_BuddhistMonk::DisableWeaponHitbox()
{
	Super::DisableWeaponHitbox();

	RightFootHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftFootHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Fire a projectile towards the target, calculating the lead based on the target's velocity to increase hit accuracy
void ANormalEnemy_Stage2_BuddhistMonk::FireProjectile_Implementation()
{
	if (!ProjectileClass) return;

	FVector MuzzleLocation = LeftWeaponMesh->GetSocketLocation(MuzzleSocketName);
	FRotator FireRotation = LeftWeaponMesh->GetSocketRotation(MuzzleSocketName);

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
		Cast<AProjectile>(Projectile)->SetDamage(BaseDamage * multiplier);

		UProjectileMovementComponent* ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
		if (ProjectileMovement)
		{
			ProjectileMovement->SetVelocityInLocalSpace(FVector::ForwardVector * ProjectileMovement->InitialSpeed);
			ProjectileMovement->Activate();
		}
	}
}