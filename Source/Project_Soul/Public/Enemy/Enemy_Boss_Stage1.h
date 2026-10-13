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
	FTimerHandle UpdateDistanceTimerHandle;

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	void UpdateDistance();

	virtual void DecideNextAction() override;

	virtual void ResumeCombat() override;

	virtual void Die() override;
};