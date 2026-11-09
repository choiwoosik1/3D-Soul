#include "Widgets/CheckpointMenuWidget.h"
#include "Widgets/CheckpointEntryWidget.h"
#include "Checkpoint.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Called when the widget is first created, before it is added to the viewport
void UCheckpointMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FastTravelButton->OnClicked.AddDynamic(this, &UCheckpointMenuWidget::ShowTravelList);
	BackButton->OnClicked.AddDynamic(this, &UCheckpointMenuWidget::ShowMainMenu);
	CloseButton->OnClicked.AddDynamic(this, &UCheckpointMenuWidget::Close);
}

// Called when the widget is added to the viewport
void UCheckpointMenuWidget::Open(ACheckpoint* InCheckpoint)
{
	Current = InCheckpoint;
	ShowMainMenu();
	AddToViewport();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeUIOnly());

		// Keys held while the menu opens never receive a release event, so clear them
		PC->FlushPressedKeys();
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}
}

// Called when the widget is removed from the viewport
void UCheckpointMenuWidget::Close()
{
	RemoveFromParent();
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

// Show the main menu panel and hide the travel list panel
void UCheckpointMenuWidget::ShowMainMenu()
{
	MainPanel->SetVisibility(ESlateVisibility::Visible);
	TravelPanel->SetVisibility(ESlateVisibility::Collapsed);
}

// Show a list of all activated checkpoints, except the current one
void UCheckpointMenuWidget::ShowTravelList()
{
	EntryContainer->ClearChildren();

	TArray<AActor*> All;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckpoint::StaticClass(), All);

	// Order the list by each checkpoint's SortOrder
	All.StableSort([](const AActor& A, const AActor& B)
		{
			return CastChecked<ACheckpoint>(&A)->GetSortOrder() < CastChecked<ACheckpoint>(&B)->GetSortOrder();
		});

	// Create an entry for each checkpoint, except the current one and any that are not activated
	for (AActor* Actor : All)
	{
		ACheckpoint* Checkpoint = Cast<ACheckpoint>(Actor);
		if (Checkpoint == Current.Get() || !Checkpoint->IsActivated()) continue;

		UCheckpointEntryWidget* Entry = CreateWidget<UCheckpointEntryWidget>(this, EntryClass);
		Entry->Setup(Checkpoint);
		Entry->OnSelected.BindUObject(this, &UCheckpointMenuWidget::OnDestinationSelected);
		EntryContainer->AddChild(Entry);
	}

	MainPanel->SetVisibility(ESlateVisibility::Collapsed);
	TravelPanel->SetVisibility(ESlateVisibility::Visible);
}

// Called when the player selects a destination checkpoint from the travel list
void UCheckpointMenuWidget::OnDestinationSelected(ACheckpoint* Destination)
{
	APawn* Player = GetOwningPlayerPawn();
	Close();

	if (Player && Destination)
	{
		Destination->TeleportPlayerToCheckpoint(Player);
	}
}