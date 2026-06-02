#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "NormalEnemy.generated.h"

class APatrolPath;

// Normal enemy class with patrol and perception systems
UCLASS()
class PROJECT_SOUL_API ANormalEnemy : public AEnemy
{
	GENERATED_BODY()

public: 
	ANormalEnemy();

protected:
	virtual void BeginPlay() override;

	// Patrol system
	UPROPERTY(EditAnywhere, Category = "Patrol")
	APatrolPath* PatrolPath = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	int32 CurrentPatrolIndex = 0;

	// Perception stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float SightRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float LoseSightRadius = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float FOVAngle = 135.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float HearingRange = 1000.f;

public:
	// Getters for stats and state
	class UBehaviorTree* GetBehaviorTreeAsset() const { return BTAsset; }

	APatrolPath* GetPatrolPath() const { return PatrolPath; }

	float GetSightRadius() const { return SightRadius; }

	float GetLoseSightRadius() const { return LoseSightRadius; }

	float GetFOVAngle() const { return FOVAngle; }

	float GetHearingRange() const { return HearingRange; }

	FVector GetNextPatrolPoint();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Die() override;

	UFUNCTION()
	void EnableRagdoll();
};
