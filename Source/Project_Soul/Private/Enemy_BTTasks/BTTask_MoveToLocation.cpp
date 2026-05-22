#include "Enemy_BTTasks/BTTask_MoveToLocation.h"
#include "Enemy/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

EBTNodeResult::Type UBTTask_MoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
	if (!BBComp) return EBTNodeResult::Failed;

	FVector Destination = BBComp->GetValueAsVector(FName("Destination"));

	AIC->MoveToLocation(Destination);
	return EBTNodeResult::Succeeded;
}