#include "Enemy_BTTasks/BTTask_FindPatrolLocation.h"
#include "Enemy/EnemyAIController.h"
#include "Enemy/NormalEnemy.h"
#include "Enemy/PatrolPath.h"
#include "BehaviorTree/BlackboardComponent.h"

// Find the next patrol location from the enemy's patrol path and store it in the blackboard
EBTNodeResult::Type UBTTask_FindPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AEnemyAIController* Controller = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    ANormalEnemy* Enemy = Controller ? Cast<ANormalEnemy>(Controller->GetPawn()) : nullptr;

    if (!Enemy || !Enemy->GetPatrolPath()) return EBTNodeResult::Failed;

    FVector NextPoint = Enemy->GetNextPatrolPoint();

    OwnerComp.GetBlackboardComponent()->SetValueAsVector(FName("Destination"), NextPoint);

    return EBTNodeResult::Succeeded;
}