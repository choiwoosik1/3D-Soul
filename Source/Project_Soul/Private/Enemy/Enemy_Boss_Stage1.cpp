#include "Enemy/Enemy_Boss_Stage1.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

AEnemy_Boss_Stage1::AEnemy_Boss_Stage1()
{
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemy_Boss_Stage1::BeginPlay()
{
	Super::BeginPlay();

	AAIController* AIC = Cast<AAIController>(GetController());
	AIC->RunBehaviorTree(BTAsset);
}

void AEnemy_Boss_Stage1::StartBossFight()
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	AIC->SetFocus(Player);

	if (UBlackboardComponent* BBComp = AIC->GetBlackboardComponent())
	{
		BBComp->SetValueAsObject(FName("TargetActor"), Player);
	}

	EnterCombat();
}

// Base decision logic for enemy's next action
void AEnemy_Boss_Stage1::DecideNextAction()
{
	if (CharacterState != EEnemyState::InCombat) return;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	AActor* Target = Cast<AActor>(AIC->GetFocusActor());
	if (!Target) return;

	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	// Find valid attack patterns based on distance to the target
	TArray<int32> ValidAttackPatterns;
	for (int32 i = 0; i < Patterns.Num(); i++)
	{
		if (Distance <= Patterns[i].AttackRange)
			ValidAttackPatterns.Add(i);
	}

	// If there are valid attack patterns, randomly select one and perform the attack
	if (ValidAttackPatterns.Num() > 0)
	{
		int32 SelectedIdx = ValidAttackPatterns[FMath::RandRange(0, ValidAttackPatterns.Num() - 1)];
		PerformAttackPattern(SelectedIdx);
		return;
	}
}