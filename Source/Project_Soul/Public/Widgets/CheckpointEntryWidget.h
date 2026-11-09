#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CheckpointEntryWidget.generated.h"

class ACheckpoint;
class UButton;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FOnCheckpointSelected, ACheckpoint*);

// Widget class for an individual checkpoint entry in the travel list, allowing players to select a destination checkpoint
UCLASS(Abstract)
class PROJECT_SOUL_API UCheckpointEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnCheckpointSelected OnSelected;

	void Setup(ACheckpoint* InTarget);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CheckpointNameText;

	TWeakObjectPtr<ACheckpoint> Target;

	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleClicked();
};