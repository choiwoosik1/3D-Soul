#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy.h"
#include "NormalEnemy_Stage3_Peasant.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Stage3_Peasant : public ANormalEnemy
{
	GENERATED_BODY()
	
public:
	ANormalEnemy_Stage3_Peasant();

protected:
	virtual void PostInitializeComponents() override;

	virtual void DecideNextAction() override;
};
