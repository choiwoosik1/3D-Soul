#include "Enemy_BTTasks/BTTask_EnemyDecideAction.h"
#include "Enemy/EnemyAIController.h"
#include "Enemy/Enemy.h"

EBTNodeResult::Type UBTTask_EnemyDecideAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AEnemyAIController* Controller = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
    AEnemy* Enemy = Controller ? Cast<AEnemy>(Controller->GetPawn()) : nullptr;

    if (!Enemy) return EBTNodeResult::Failed;

    Enemy->DecideNextAction();
    return EBTNodeResult::Succeeded;
}