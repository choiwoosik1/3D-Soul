#include "Enemy_BTTasks/BTDecorator_WithinCombatRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTDecorator_WithinCombatRange::UBTDecorator_WithinCombatRange()
{
	NodeName = "Within Combat Range";
    bNotifyBecomeRelevant = true;
    bNotifyCeaseRelevant = true;
}

void UBTDecorator_WithinCombatRange::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent())
    {
        FBlackboard::FKey KeyID = BBComp->GetKeyID(FName("DistanceToTarget"));
        BBComp->RegisterObserver(KeyID, this,
            FOnBlackboardChangeNotification::CreateUObject(
                this, &UBTDecorator_WithinCombatRange::OnBlackboardValueChange));
    }
}

void UBTDecorator_WithinCombatRange::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent())
    {
        BBComp->UnregisterObserversFrom(this);
    }
}

EBlackboardNotificationResult UBTDecorator_WithinCombatRange::OnBlackboardValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID)
{
    UBehaviorTreeComponent* BehaviorComp =
        Cast<UBehaviorTreeComponent>(Blackboard.GetBrainComponent());
    if (!BehaviorComp) return EBlackboardNotificationResult::RemoveObserver;

    float DistanceToTarget = Blackboard.GetValueAsFloat(FName("DistanceToTarget"));
    float MaxCombatRange = Blackboard.GetValueAsFloat(FName("MaxCombatRange"));
    bool bNewResult = DistanceToTarget <= MaxCombatRange;

    if (bNewResult != bLastResult)
    {
        bLastResult = bNewResult;
        BehaviorComp->RequestExecution(this);
    }

    return EBlackboardNotificationResult::ContinueObserving;
}

bool UBTDecorator_WithinCombatRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    if (!BBComp) return false;

    float DistanceToTarget = BBComp->GetValueAsFloat(FName("DistanceToTarget"));
    float MaxCombatRange = BBComp->GetValueAsFloat(FName("MaxCombatRange"));

    bLastResult = DistanceToTarget <= MaxCombatRange;
    return bLastResult;
}