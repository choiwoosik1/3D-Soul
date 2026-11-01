#include "Enemy/NormalEnemy_Stage2_Korea.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "AIController.h"

// Sets default values
ANormalEnemy_Stage2_Korea::ANormalEnemy_Stage2_Korea()
{
	RightWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWeaponMesh"));
	LeftWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWeaponMesh"));
}

// Called after the actor's components have been initialized
void ANormalEnemy_Stage2_Korea::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Attach the weapon mesh to the character's hand socket
	RightWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightWeaponSocketName);
	RightWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage2_Korea::OnWeaponHitboxOverlap);

	LeftWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, LeftWeaponSocketName);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage2_Korea::DecideNextAction()
{
	Super::DecideNextAction();
}