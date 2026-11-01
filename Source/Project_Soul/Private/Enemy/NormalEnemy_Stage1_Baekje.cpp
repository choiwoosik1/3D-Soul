#include "Enemy/NormalEnemy_Stage1_Baekje.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "AIController.h"

// Sets default values
ANormalEnemy_Stage1_Baekje::ANormalEnemy_Stage1_Baekje()
{
	RightWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWeaponMesh"));
}

// Called after the actor's components have been initialized
void ANormalEnemy_Stage1_Baekje::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Attach the weapon mesh to the character's hand socket
	RightWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightWeaponSocketName);
	RightWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage1_Baekje::OnWeaponHitboxOverlap);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage1_Baekje::DecideNextAction()
{
	Super::DecideNextAction();
}