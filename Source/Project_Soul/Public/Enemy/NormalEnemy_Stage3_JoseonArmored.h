#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy.h"
#include "NormalEnemy_Stage3_JoseonArmored.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Stage3_JoseonArmored : public ANormalEnemy
{
	GENERATED_BODY()
	
public:
	ANormalEnemy_Stage3_JoseonArmored();

protected:
	virtual void PostInitializeComponents() override;

	virtual void DecideNextAction() override;
};
