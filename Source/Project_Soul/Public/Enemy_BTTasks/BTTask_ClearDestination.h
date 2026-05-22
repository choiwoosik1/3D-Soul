#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearDestination.generated.h"

UCLASS()
class PROJECT_SOUL_API UBTTask_ClearDestination : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ClearDestination();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};