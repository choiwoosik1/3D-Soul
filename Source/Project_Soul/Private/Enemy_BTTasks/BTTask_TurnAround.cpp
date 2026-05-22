#include "Enemy_BTTasks/BTTask_TurnAround.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Enemy/Enemy.h"

EBTNodeResult::Type UBTTask_TurnAround::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return EBTNodeResult::Failed;

    AEnemy* Enemy = Cast<AEnemy>(AIController->GetPawn());
    if (!Enemy) return EBTNodeResult::Failed;
	if (Enemy->GetCharacterState() != EEnemyState::InCombat || 
        Enemy->GetCharacterState() != EEnemyState::Alert) return EBTNodeResult::Failed;

    FRotator NewRotation = Enemy->GetActorRotation();
    NewRotation.Yaw += 180.0f;
    Enemy->SetActorRotation(NewRotation);

    return EBTNodeResult::Succeeded;
}