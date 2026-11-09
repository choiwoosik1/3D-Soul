#include "Widgets/CheckpointEntryWidget.h"
#include "Checkpoint.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// Called when the widget is first created, before it is added to the viewport
void UCheckpointEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SelectButton->OnClicked.AddDynamic(this, &UCheckpointEntryWidget::HandleClicked);
}

// Set up the checkpoint entry with the target checkpoint's name and store a reference to it
void UCheckpointEntryWidget::Setup(ACheckpoint* InTarget)
{
	Target = InTarget;
	CheckpointNameText->SetText(InTarget->GetCheckpointName());
}

// Called when the player clicks the select button for this checkpoint entry, triggering the OnSelected delegate
void UCheckpointEntryWidget::HandleClicked()
{
	OnSelected.ExecuteIfBound(Target.Get());
}