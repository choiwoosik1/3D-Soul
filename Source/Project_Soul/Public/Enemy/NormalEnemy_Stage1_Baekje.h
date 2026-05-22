#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy.h"
#include "NormalEnemy_Stage1_Baekje.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Stage1_Baekje : public ANormalEnemy
{
	GENERATED_BODY()
	
public:
	ANormalEnemy_Stage1_Baekje();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	class USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	class UBoxComponent* WeaponHitbox;

	virtual void BeginPlay() override;

	virtual void DecideNextAction() override;

	virtual void EnableWeaponHitbox() override;

	virtual void DisableWeaponHitbox() override;

	virtual void Die() override;
};
