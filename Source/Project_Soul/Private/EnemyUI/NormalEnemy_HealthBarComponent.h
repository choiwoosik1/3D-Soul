#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "NormalEnemy_HealthBarComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_SOUL_API UNormalEnemy_HealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UNormalEnemy_HealthBarComponent();

	// ANormalEnemy::NotifyHealthBarUpdate 에서 호출
	void UpdateHealth(float CurrentHealth, float MaxHealth);

	// ANormalEnemy::NotifyHealthBarLockOn 에서 호출
	void SetLockOnState(bool bIsLockedOn);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "HealthBar")
	float AutoHideDelay = 7.0f;

	UPROPERTY(EditAnywhere, Category = "HealthBar")
	float DeathHideDelay = 3.0f;

	UPROPERTY(VisibleAnywhere, Category = "HealthBar")
	bool bIsDead = false;

private:
	void RestartAutoHideTimer();
	void OnAutoHideTimerExpired();

	bool bIsLockedOn = false;

	FTimerHandle AutoHideTimerHandle;
};