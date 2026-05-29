#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Enemy_Boss_Stage1.generated.h"

UCLASS()
class PROJECT_SOUL_API AEnemy_Boss_Stage1 : public AEnemy
{
	GENERATED_BODY()
	
public:
	AEnemy_Boss_Stage1();

protected:
	FTimerHandle UpdateSpeedTimerHandle;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void StartBossFight();

	virtual void DecideNextAction() override;

	void UpdateSpeed();
};