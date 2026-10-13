#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "EnemyUI/Boss_HealthBarWidget.h"
#include "Enemy_Boss_Stage1.generated.h"

UCLASS()
class PROJECT_SOUL_API AEnemy_Boss_Stage1 : public AEnemy
{
	GENERATED_BODY()
	
public:
	AEnemy_Boss_Stage1();

	UFUNCTION(BlueprintCallable)
	void StartBossFight();

	void EndBossFight();

protected:
	FTimerHandle UpdateDistanceTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBoss_HealthBarWidget> BossHealthBarClass;

	UPROPERTY()
	TObjectPtr<UBoss_HealthBarWidget> BossHealthBarWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText BossDisplayName;

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	virtual void NotifyHealthBarUpdate(float NewCurrentHealth, float NewMaxHealth) override;

	void UpdateDistance();

	virtual void DecideNextAction() override;

	virtual void ResumeCombat() override;
};