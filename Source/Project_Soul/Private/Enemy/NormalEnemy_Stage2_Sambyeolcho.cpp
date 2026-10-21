#include "Enemy/NormalEnemy_Stage2_Sambyeolcho.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "AIController.h"

// Sets default values
ANormalEnemy_Stage2_Sambyeolcho::ANormalEnemy_Stage2_Sambyeolcho()
{
	RightWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWeaponMesh"));
}

// Called after the actor's components have been initialized
void ANormalEnemy_Stage2_Sambyeolcho::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Attach the weapon mesh to the character's hand socket
	RightWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightWeaponSocketName);
	RightWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage2_Sambyeolcho::OnWeaponHitboxOverlap);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage2_Sambyeolcho::DecideNextAction()
{
	Super::DecideNextAction();
}