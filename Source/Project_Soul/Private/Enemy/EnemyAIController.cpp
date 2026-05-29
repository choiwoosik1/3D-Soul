#include "Enemy/EnemyAIController.h"
#include "Enemy/NormalEnemy.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
AEnemyAIController::AEnemyAIController()
{
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

    AIPerceptionComponent->ConfigureSense(*SightConfig);
    AIPerceptionComponent->ConfigureSense(*HearingConfig);
    AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

    AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetDetected);
}

// Called when the controller possesses a pawn
void AEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (ANormalEnemy* Enemy = Cast<ANormalEnemy>(InPawn))
    {
        if (Enemy->GetBehaviorTreeAsset())
        {
            RunBehaviorTree(Enemy->GetBehaviorTreeAsset());
        }

        // Initialize perception config based on enemy properties
        SightConfig->SightRadius = Enemy->GetSightRadius();
        SightConfig->LoseSightRadius = Enemy->GetLoseSightRadius();
        SightConfig->PeripheralVisionAngleDegrees = Enemy->GetFOVAngle() / 2.f;
        SightConfig->DetectionByAffiliation.bDetectEnemies = false;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
        AIPerceptionComponent->ConfigureSense(*SightConfig);

        HearingConfig->HearingRange = Enemy->GetHearingRange();
		HearingConfig->DetectionByAffiliation.bDetectEnemies = false;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
        AIPerceptionComponent->ConfigureSense(*HearingConfig);

		AIPerceptionComponent->RequestStimuliListenerUpdate();
    }
}

// Handle perception updates for sight and hearing, updating the blackboard and enemy state
void AEnemyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
    if (Cast<AEnemy>(Actor)) return;

    UBlackboardComponent* BBComp = GetBlackboardComponent();
    if (!BBComp) return;

    if (Stimulus.WasSuccessfullySensed())
    {
		DetectedTarget = Actor;

	    // Handle sight updates
        if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
        {
            bSightDetected = true;
        }

		// Handle hearing updates
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
        {
			bHearingDetected = true;
			LastHearingStimulus = Stimulus;
        }

		// If enemy is idle or patrolling, transition to alert state
        AEnemy* Enemy = Cast<AEnemy>(GetPawn());
        if (!Enemy) return;
        EEnemyState state = Enemy->GetCharacterState();

        if (state == EEnemyState::Idle || state== EEnemyState::Patrol)
        {
            Enemy->EnterAlert();
        }
        
        // Start or continue updating detection level
        if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
        {
            GetWorldTimerManager().SetTimer(DetectionTimerHandle, this,
                &AEnemyAIController::UpdateDetectionLevel, 0.1f, true);
        }
    }
	// Lost sight of target, update blackboard and start losing detection
    else
    {
        if (DetectedTarget)
        {
            BBComp->SetValueAsVector(FName("Destination"), DetectedTarget->GetActorLocation());
        }
		DetectedTarget = nullptr;
        bSightDetected = false;
        BBComp->ClearValue(FName("TargetActor"));
		ClearFocus(EAIFocusPriority::Gameplay);      
    }
}

// Update the detection level based and transition enemy state
void AEnemyAIController::UpdateDetectionLevel()
{
    ANormalEnemy* Enemy = Cast<ANormalEnemy>(GetPawn());
    if (!Enemy) return; 
    
    UBlackboardComponent* BBComp = GetBlackboardComponent();
	if (!BBComp) return;

    if (DetectedTarget)
    {
        float Distance = FVector::Dist(Enemy->GetActorLocation(), DetectedTarget->GetActorLocation());
        float SightIncrease = 0.f;
		float HearingIncrease = 0.f;

        if (bSightDetected)
        {
            float DistanceRatio = FMath::Clamp(Distance / Enemy->GetSightRadius(), 0.f, 1.f);
            float SpeedMultiplier = FMath::Lerp(6.f, 1.f, DistanceRatio);
            SightIncrease = SightIncreaseRate * SpeedMultiplier;
		}

        if (bHearingDetected)
        {
            float DistanceRatio = FMath::Clamp(Distance / Enemy->GetHearingRange(), 0.f, 1.f);
            float SpeedMultiplier = FMath::Lerp(10.f, 1.f, DistanceRatio);
            HearingIncrease = HearingIncreaseRate * SpeedMultiplier * LastHearingStimulus.Strength;
            bHearingDetected = false;
        }

		float CurrentRate = SightIncrease + HearingIncrease;

		// Increase detection level while target is in sight
        DetectionLevel = FMath::Clamp(DetectionLevel + CurrentRate * 0.1f, 0.f, MaxDetectionLevel);

		// Update blackboard with target location only if no target is currently set
        UObject* TargetObject = BBComp->GetValueAsObject(FName("TargetActor"));
        if (TargetObject == nullptr)
        {
            BBComp->SetValueAsVector(FName("Destination"), DetectedTarget->GetActorLocation());
        }

		// Rotate enemy to face the target when detection level reaches certain thresholds
		bool bCrossed30 = PrevDetectionLevel < 0.3f * MaxDetectionLevel && 
            DetectionLevel >= 0.3f * MaxDetectionLevel;
        bool bCrossed40 = PrevDetectionLevel < 0.4f * MaxDetectionLevel && 
			DetectionLevel >= 0.4f * MaxDetectionLevel;
		PrevDetectionLevel = DetectionLevel;

        if ((bCrossed30 || bCrossed40) && DetectedTarget)
        {
            SetFocus(DetectedTarget);
            FTimerHandle TempHandle;
            GetWorldTimerManager().SetTimer(TempHandle, [this]()
                { ClearFocus(EAIFocusPriority::Gameplay); }, 0.6f, false);
        }

		// Transition to combat state
        if (DetectionLevel >= MaxDetectionLevel && Enemy->GetCharacterState() == EEnemyState::Alert)
        {
            Enemy->EnterCombat();
        }

		// If in combat, set target and focus
        if (Enemy->GetCharacterState() == EEnemyState::InCombat)
        {
            DetectionLevel = 100.f;
            SetFocus(DetectedTarget);
            BBComp->SetValueAsObject(FName("TargetActor"), DetectedTarget);
			BBComp->SetValueAsFloat(FName("DistanceToTarget"), Distance);
			Enemy->SetSpeedByDistance(Distance);
		}

        if (!bSightDetected)
        {
            DetectedTarget = nullptr;
        }
    }
	// Decrease detection level when target is lost
    else
    {
		float CurrentDecreaseRate = DetectionDecreaseRate;

		// If in combat, decrease detection level more slowly and increase movement speed
		if (Enemy->GetCharacterState() == EEnemyState::InCombat)
        {
            Enemy->SetSpeedByDistance(1000.f);
            CurrentDecreaseRate *= 0.5f;
        }

        DetectionLevel = FMath::Clamp(DetectionLevel - CurrentDecreaseRate * 0.1f, 0.f, MaxDetectionLevel);

		// If detection level drops below half while in combat, transition to alert state
		if (DetectionLevel < 0.5f * MaxDetectionLevel && Enemy->GetCharacterState() == EEnemyState::InCombat)
        {
            Enemy->EnterAlert();
        }

		// If detection level drops to 0, clear timer and transition back to patrol
        if (DetectionLevel <= 0.f)
        {
            GetWorldTimerManager().ClearTimer(DetectionTimerHandle);
			BBComp->ClearValue(FName("Destination"));
            Enemy->EnterPatrol();
        }
    }

	// Update blackboard with current detection level
    BBComp->SetValueAsFloat(FName("DetectionLevel"), DetectionLevel);
}

void AEnemyAIController::TargetDetected(AActor* Actor)
{
    UBlackboardComponent* BBComp = GetBlackboardComponent();
    if (BBComp)
    {
		DetectionLevel = MaxDetectionLevel;
		FVector Destination = Actor->GetActorLocation() - Actor->GetActorForwardVector() * 5.f;
        BBComp->SetValueAsVector("Destination", Destination);
        BBComp->SetValueAsFloat("DetectionLevel", DetectionLevel);
    }
}