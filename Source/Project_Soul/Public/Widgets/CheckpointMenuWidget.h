#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CheckpointMenuWidget.generated.h"

class ACheckpoint;
class UButton;
class UPanelWidget;
class UCheckpointEntryWidget;

// Widget class for the checkpoint menu, allowing players to fast travel between activated checkpoints
UCLASS(Abstract)
class PROJECT_SOUL_API UCheckpointMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void Open(ACheckpoint* InCheckpoint);

protected:
	// Main menu screen
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> MainPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FastTravelButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	// Destination list screen
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> TravelPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> EntryContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCheckpointEntryWidget> EntryClass;

	UPROPERTY(EditDefaultsOnly)
	float EntrySpacing = 20.f;

	TWeakObjectPtr<ACheckpoint> Current;

	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void ShowMainMenu();

	UFUNCTION()
	void ShowTravelList();

	UFUNCTION()
	void Close();

	void OnDestinationSelected(ACheckpoint* Destination);
};
