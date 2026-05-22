#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolLocation.generated.h"

// Task node to find the next patrol location for the enemy and store it in the blackboard
UCLASS()
class PROJECT_SOUL_API UBTTask_FindPatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()
	
protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
