#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy_Ranged.h"
#include "NormalEnemy_Stage2_BuddhistMonk.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Stage2_BuddhistMonk : public ANormalEnemy_Ranged
{
	GENERATED_BODY()
	
public:
	ANormalEnemy_Stage2_BuddhistMonk();

protected:

	UPROPERTY(EditAnywhere, Category = "Weapon")
	UBoxComponent* RightFootHitbox;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	FName RightFootSocketName = TEXT("ball_r");

	UPROPERTY(EditAnywhere, Category = "Weapon")
	UBoxComponent* LeftFootHitbox;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	FName LeftFootSocketName = TEXT("ball_l");

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileSpeed = 800.f;

	virtual void PostInitializeComponents() override;

	virtual void DecideNextAction() override;

	virtual void EnableWeaponHitbox() override;

	virtual void DisableWeaponHitbox() override;

	virtual void FireProjectile_Implementation() override;
};
