#include "Enemy/NormalEnemy_Stage2_BuddhistMonk.h"
#include "Enemy/Projectile.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
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

}