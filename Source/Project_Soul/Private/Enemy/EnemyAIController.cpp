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
    // Create and initialize perception components
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

	// Configure sight and hearing senses with default values, allowing for dynamic updates based on enemy properties
    AIPerceptionComponent->ConfigureSense(*SightConfig);
    AIPerceptionComponent->ConfigureSense(*HearingConfig);
    AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	// Bind perception update events to handle target detection and loss for both sight and hearing
    AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetDetected);
}

// Called when the controller possesses a pawn
void AEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (ANormalEnemy* Enemy = Cast<ANormalEnemy>(InPawn))
    {
		// Configure sight sense based on enemy properties, allowing for dynamic updates if needed
        SightConfig->SightRadius = Enemy->GetSightRadius();
        SightConfig->LoseSightRadius = Enemy->GetLoseSightRadius();
        SightConfig->PeripheralVisionAngleDegrees = Enemy->GetFOVAngle() / 2.f;
        SightConfig->DetectionByAffiliation.bDetectEnemies = false;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
        AIPerceptionComponent->ConfigureSense(*SightConfig);

        // Configure hearing sense based on enemy properties, allowing for dynamic updates if needed
        HearingConfig->HearingRange = Enemy->GetHearingRange();
		HearingConfig->DetectionByAffiliation.bDetectEnemies = false;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
        AIPerceptionComponent->ConfigureSense(*HearingConfig);

		// Request an update to the stimuli listener to ensure the AI perception system is aware of the new configurations
		AIPerceptionComponent->RequestStimuliListenerUpdate();

		// Initialize the behavior tree and blackboard with enemy-specific properties, allowing for dynamic updates if needed
        if (!Enemy->GetBehaviorTreeAsset()) return;
        RunBehaviorTree(Enemy->GetBehaviorTreeAsset());

		UBlackboardComponent* BBComp = GetBlackboardComponent();
        BBComp->SetValueAsFloat(FName("MinCombatRange"),Enemy->GetMinCombatRange());
        BBComp->SetValueAsFloat(FName("MaxCombatRange"),Enemy->GetMaxCombatRange());
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
        ANormalEnemy* Enemy = Cast<ANormalEnemy>(GetPawn());
        if (!Enemy) return;

        EEnemyState state = Enemy->GetCharacterState();
        if (state == EEnemyState::Idle || state== EEnemyState::Patrol)
        {
            Enemy->EnterAlert();
        }
        
        // Start or continue updating detection level
        if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
        {
            GetWorldTimerManager().SetTimer(DetectionTimerHandle, this, &AEnemyAIController::UpdateDetectionLevel, 0.1f, true);
        }
    }
	// Lost sight of target, update blackboard and start losing detection
    else
    {
        if (DetectedTarget)
        {
            BBComp->SetValueAsVector(FName("Destination"), DetectedTarget->GetActorLocation());
		    DetectedTarget = nullptr;
        }
        BBComp->ClearValue(FName("TargetActor"));
		ClearFocus(EAIFocusPriority::Gameplay);
        bSightDetected = false;      
    }
}

// Update the detection level based and transition enemy state
void AEnemyAIController::UpdateDetectionLevel()
{
    ANormalEnemy* Enemy = Cast<ANormalEnemy>(GetPawn());
    if (!Enemy) return; 
    
    UBlackboardComponent* BBComp = GetBlackboardComponent();
	if (!BBComp) return;

    // Calculate detection increase based on sight and hearing, adjusting for distance and stimulus strength
    if (DetectedTarget)
    {
        float Distance = FVector::Dist(Enemy->GetActorLocation(), DetectedTarget->GetActorLocation());
        float SightIncrease = 0.f;
		float HearingIncrease = 0.f;

		// Increase detection level based on sight, with faster increase when target is closer
        if (bSightDetected)
        {
            float DistanceRatio = FMath::Clamp(Distance / Enemy->GetSightRadius(), 0.f, 1.f);
            float SpeedMultiplier = FMath::Lerp(6.f, 1.f, DistanceRatio);
            SightIncrease = SightIncreaseRate * SpeedMultiplier;
		}

		// Increase detection level based on hearing, with faster increase when target is closer and stronger stimulus
        if (bHearingDetected)
        {
            float DistanceRatio = FMath::Clamp(Distance / Enemy->GetHearingRange(), 0.f, 1.f);
            float SpeedMultiplier = FMath::Lerp(10.f, 1.f, DistanceRatio);
            HearingIncrease = HearingIncreaseRate * SpeedMultiplier * LastHearingStimulus.Strength;
            bHearingDetected = false;
        }

        DetectionLevel = FMath::Clamp(DetectionLevel + SightIncrease + HearingIncrease, 0.f, MaxDetectionLevel);

		// Update blackboard with target location only if the enemy is not in combat thus no target is currently set
        UObject* TargetObject = BBComp->GetValueAsObject(FName("TargetActor"));
        if (TargetObject == nullptr)
        {
            BBComp->SetValueAsVector(FName("Destination"), DetectedTarget->GetActorLocation());
        }

		// Rotate enemy to face the target when detection level reaches certain thresholds
		bool bCrossed30 = PrevDetectionLevel < 0.3f * MaxDetectionLevel && DetectionLevel >= 0.3f * MaxDetectionLevel;
        bool bCrossed40 = PrevDetectionLevel < 0.4f * MaxDetectionLevel && DetectionLevel >= 0.4f * MaxDetectionLevel;
		PrevDetectionLevel = DetectionLevel;

        if ((bCrossed30 || bCrossed40) && DetectedTarget)
        {
            SetFocus(DetectedTarget);
            FTimerHandle TempHandle;
            GetWorldTimerManager().SetTimer(TempHandle, [this]() { ClearFocus(EAIFocusPriority::Gameplay); }, 0.6f, false);
        }

		// Transition to combat state when detection level reaches max and enemy is alert
        if (DetectionLevel >= MaxDetectionLevel && Enemy->GetCharacterState() == EEnemyState::Alert)
        {
            Enemy->EnterCombat();
        }

		// If in combat and target is detected, set focus on the target and update blackboard with target info and distance
        if (Enemy->GetCharacterState() == EEnemyState::InCombat)
        {
            DetectionLevel = 100.f;
            SetFocus(DetectedTarget);
            BBComp->SetValueAsObject(FName("TargetActor"), DetectedTarget);
			BBComp->SetValueAsFloat(FName("DistanceToTarget"), Distance);
			Enemy->SetSpeedByDistance(Distance);
		}

		// If hearing was the only sense that detected the target, clear it after processing to allow for new stimuli
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
            CurrentDecreaseRate *= 0.5f;
            Enemy->SetSpeedByDistance(1.5f * Enemy->GetMaxCombatRange());
        }

        DetectionLevel = FMath::Clamp(DetectionLevel - CurrentDecreaseRate * 0.1f, 0.f, MaxDetectionLevel);

		// If detection level drops below half while in combat, transition to alert state ?
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

// Handle immediate target detection during combat, setting destination and resetting detection level
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