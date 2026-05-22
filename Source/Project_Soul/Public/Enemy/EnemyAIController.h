#pragma once

#include "AIController.h"
#include "Enemy/EnemyTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

UCLASS()
class PROJECT_SOUL_API AEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyAIController();

protected:
	// Perception System
    UPROPERTY(VisibleAnywhere, Category = "Perception")
    class UAIPerceptionComponent* AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Perception")
    class UAISenseConfig_Sight* SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "Perception")
	class UAISenseConfig_Hearing* HearingConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception")
	float DetectionLevel = 0.f; 

	float PrevDetectionLevel = 0.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
    float MaxDetectionLevel = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
    float SightIncreaseRate = 30.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
    float HearingIncreaseRate = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
    float DetectionDecreaseRate = 2.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception")
	bool bSightDetected = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception")
	bool bHearingDetected = false;

	FAIStimulus LastHearingStimulus;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception")
	AActor* DetectedTarget = nullptr;

	FTimerHandle DetectionTimerHandle;

    virtual void OnPossess(APawn* InPawn) override;

    UFUNCTION()
    void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

	void UpdateDetectionLevel();

public:
	void TargetDetected(AActor* Actor);
};