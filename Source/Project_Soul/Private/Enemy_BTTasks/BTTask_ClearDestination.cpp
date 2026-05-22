#include "Enemy_BTTasks/BTTask_ClearDestination.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearDestination::UBTTask_ClearDestination()
{
    NodeName = TEXT("Clear Last Known Location");
}

EBTNodeResult::Type UBTTask_ClearDestination::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    if (!BBComp) return EBTNodeResult::Failed;
    BBComp->ClearValue(FName("Destination"));
    return EBTNodeResult::Succeeded;
}