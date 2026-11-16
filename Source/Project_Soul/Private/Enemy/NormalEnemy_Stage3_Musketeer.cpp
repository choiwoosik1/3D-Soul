#include "Enemy/NormalEnemy_Stage3_Musketeer.h"
#include "Enemy/Projectile.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ANormalEnemy_Stage3_Musketeer::ANormalEnemy_Stage3_Musketeer()
{
	LeftWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWeaponMesh"));
}

// Called after the actor's components have been initialized
void ANormalEnemy_Stage3_Musketeer::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Attach the weapon mesh to the character's hand socket
	LeftWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, LeftWeaponSocketName);
	LeftWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage3_Musketeer::OnWeaponHitboxOverlap);
}

// Fire a projectile towards the target, calculating the lead based on the target's velocity to increase hit accuracy
void ANormalEnemy_Stage3_Musketeer::FireProjectile_Implementation()
{
	if (!ProjectileClass) return;
	if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;

	const FAttackProperties& Attack = Patterns[PatternIdx].Attacks[AttackIdx++];

	const FVector MuzzleLocation = LeftWeaponMesh->GetSocketLocation(MuzzleSocketName);
	FRotator FireRotation = LeftWeaponMesh->GetSocketRotation(MuzzleSocketName);

	AAIController* AIC = Cast<AAIController>(GetController());
	if (AIC && AIC->GetFocusActor())
	{
		FireRotation = (AIC->GetFocusActor()->GetActorLocation() - MuzzleLocation).Rotation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, MuzzleLocation, FireRotation, SpawnParams);
	if (!Projectile) return;

	Projectile->SetDamage(BaseDamage * Attack.DamageMultiplier);
	Projectile->Launch(FireRotation.Vector(), ProjectileSpeed);
}
