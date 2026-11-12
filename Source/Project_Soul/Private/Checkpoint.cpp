#include "Checkpoint.h"
#include "Widgets/CheckpointMenuWidget.h"
#include "Enemy/Enemy.h"
#include "Characters/KwangHeroCharacters.h"
#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Returns the controller only for player-controlled pawns (enemies return null)
	APlayerController* GetPlayerControllerOf(AActor* Actor)
	{
		const APawn* Pawn = Cast<APawn>(Actor);
		return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	}
}

// Constructor for the checkpoint actor, setting up mesh, respawn point, and interaction range
ACheckpoint::ACheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// In front of the checkpoint, facing it
	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(Mesh);
	RespawnPoint->SetRelativeLocation(FVector(0.f, 300.f, 0.f));
	RespawnPoint->SetRelativeRotation(FRotator(0.f, 270.f, 0.f));

	InteractRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractRange"));
	InteractRange->SetupAttachment(Mesh);
	InteractRange->SetSphereRadius(200.f);
	InteractRange->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	InteractRange->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::OnRangeEntered);
	InteractRange->OnComponentEndOverlap.AddDynamic(this, &ACheckpoint::OnRangeExited);
}

// Called when a pawn enters the interaction range, enabling input and binding the interact key
void ACheckpoint::OnRangeEntered(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerController* PC = GetPlayerControllerOf(OtherActor);
	if (!PC) return;

	// The input component is created by EnableInput, so bind the key afterwards (once)
	EnableInput(PC);
	if (!bInputBound)
	{
		InputComponent->BindKey(InteractKey, IE_Pressed, this, &ACheckpoint::Interact);
		bInputBound = true;
	}
}

// Called when a pawn exits the interaction range, disabling input for that player
void ACheckpoint::OnRangeExited(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (APlayerController* PC = GetPlayerControllerOf(OtherActor))
	{
		DisableInput(PC);
	}
}

// Called when the player interacts with the checkpoint, activating it, resetting enemies, and opening the checkpoint menu
void ACheckpoint::Interact()
{
	bActivated = true;

	// Rest the player at the checkpoint, adjusting for collision height and rotation
	if (AKwangHeroCharacters* Player = Cast<AKwangHeroCharacters>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->RestAtCheckpoint(this);
	}

	ResetEnemies();

	if (!MenuClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (UCheckpointMenuWidget* Menu = CreateWidget<UCheckpointMenuWidget>(PC, MenuClass))
	{
		Menu->Open(this);
	}
}

// Reset every enemy in the current level to its initial state
void ACheckpoint::ResetEnemies() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AEnemy::StaticClass(), Enemies);

	for (AActor* Actor : Enemies)
	{
		CastChecked<AEnemy>(Actor)->ResetToInitialState();
	}
}

// Teleports the player to the checkpoint's respawn point, adjusting for the player's collision height and setting the controller's rotation
void ACheckpoint::TeleportPlayerToCheckpoint(APawn* Player) const
{
	const FTransform Dest = RespawnPoint->GetComponentTransform();
	const FVector Location = Dest.GetLocation() + FVector(0.f, 0.f, Player->GetSimpleCollisionHalfHeight());

	Player->TeleportTo(Location, Dest.Rotator());

	if (AController* Controller = Player->GetController())
	{
		Controller->SetControlRotation(Dest.Rotator());
	}
}

void ACheckpoint::RespawnPlayerAtCheckpoint(APawn* Player) const
{
	TeleportPlayerToCheckpoint(Player);
	ResetEnemies();
}