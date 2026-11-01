#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy.h"
#include "NormalEnemy_Stage2_Sambyeolcho.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Stage2_Sambyeolcho : public ANormalEnemy
{
	GENERATED_BODY()
	
public:
	ANormalEnemy_Stage2_Sambyeolcho();

protected:
	virtual void PostInitializeComponents() override;

	virtual void DecideNextAction() override;

	UFUNCTION(BlueprintCallable)
	void EnableBackFlip();
};
