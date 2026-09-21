#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy.h"
#include "NormalEnemy_Ranged.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Ranged : public ANormalEnemy
{
	GENERATED_BODY()

public: ANormalEnemy_Ranged();

protected:
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FName MuzzleSocketName = FName("muzzle");

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void FireProjectile();

	virtual void StartDecisionTimer() override;

	virtual void SetSpeedByDistance(float Distance) override;
};