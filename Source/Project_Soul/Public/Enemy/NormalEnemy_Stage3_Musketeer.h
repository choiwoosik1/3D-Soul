#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy_Ranged.h"
#include "NormalEnemy_Stage3_Musketeer.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Stage3_Musketeer : public ANormalEnemy_Ranged
{
	GENERATED_BODY()
	
public:
	ANormalEnemy_Stage3_Musketeer();

protected:
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileSpeed = 800.f;

	virtual void PostInitializeComponents() override;

	virtual void DecideNextAction() override;

	virtual void FireProjectile_Implementation() override;
};
