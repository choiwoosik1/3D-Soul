#include "Enemy/NormalEnemy_Stage3_JoseonArmored.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "AIController.h"

// Sets default values
ANormalEnemy_Stage3_JoseonArmored::ANormalEnemy_Stage3_JoseonArmored()
{
	RightWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWeaponMesh"));
}

// Called after the actor's components have been initialized
void ANormalEnemy_Stage3_JoseonArmored::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Attach the weapon mesh to the character's hand socket
	RightWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightWeaponSocketName);
	RightWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage3_JoseonArmored::OnWeaponHitboxOverlap);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage3_JoseonArmored::DecideNextAction()
{
	Super::DecideNextAction();
}