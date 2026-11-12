#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "Checkpoint.generated.h"

class UCheckpointMenuWidget;
class USphereComponent;

// Checkpoint actor that restores player's health and mana, respawns enemies,
// and allows players to fast travel to activated checkpoints and respawn at them
UCLASS()
class PROJECT_SOUL_API ACheckpoint : public AActor
{
	GENERATED_BODY()
	
public:	
	ACheckpoint();

	FText GetCheckpointName() const { return CheckpointName; }

	bool IsActivated() const { return bActivated; }

	int32 GetSortOrder() const { return SortOrder; }

	void TeleportPlayerToCheckpoint(APawn* Player) const;

	void RespawnPlayerAtCheckpoint(APawn* Player) const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RespawnPoint;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractRange;

	UPROPERTY(EditInstanceOnly)
	FText CheckpointName;

	UPROPERTY(EditDefaultsOnly)
	FKey InteractKey = EKeys::E;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCheckpointMenuWidget> MenuClass;

	UPROPERTY(EditInstanceOnly)
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere)
	bool bActivated = false;

	bool bInputBound = false;

	UFUNCTION()
	void OnRangeEntered(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRangeExited(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void Interact();

	void ResetEnemies() const;
};