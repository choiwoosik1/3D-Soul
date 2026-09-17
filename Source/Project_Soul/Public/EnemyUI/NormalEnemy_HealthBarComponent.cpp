#include "EnemyUI/NormalEnemy_HealthBarComponent.h"
#include "EnemyUI/NormalEnemy_HealthBarWidget.h"
#include "TimerManager.h"

// Sets default values
UNormalEnemy_HealthBarComponent::UNormalEnemy_HealthBarComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(FVector2D(160.f, 14.f));
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetVisibility(false);
}

// Called when the game starts or when spawned
void UNormalEnemy_HealthBarComponent::BeginPlay()
{
	Super::BeginPlay();
	SetVisibility(false);
}

// Updates the health bar widget with the new health values and manages visibility and auto-hide behavior.
void UNormalEnemy_HealthBarComponent::UpdateHealth(float NewCurrentHealth, float NewMaxHealth)
{
	if (UNormalEnemy_HealthBarWidget* HealthBarWidget = Cast<UNormalEnemy_HealthBarWidget>(GetUserWidgetObject()))
	{
		const float Percent = NewMaxHealth > 0.f ? NewCurrentHealth / NewMaxHealth : 0.f;
		HealthBarWidget->SetHealthPercent(Percent);

		if (Percent <= 0.f)
		{
			isDead = true;
		}
	}

	SetVisibility(true);
	RestartAutoHideTimer();
}

// Sets the lock-on state of the health bar, controlling its visibility and auto-hide behavior
void UNormalEnemy_HealthBarComponent::SetLockOnState(bool bNewLockOnState)
{
	bIsLockedOn = bNewLockOnState;
	
	if (bIsLockedOn)
	{
		SetVisibility(true);
		GetWorld()->GetTimerManager().ClearTimer(AutoHideTimerHandle);
	}
	else
	{
		SetVisibility(true);
		RestartAutoHideTimer();
	}
}

// Restarts the auto-hide timer for the health bar, unless it is currently locked on.
void UNormalEnemy_HealthBarComponent::RestartAutoHideTimer()
{
	if (bIsLockedOn)
	{
		return;

	}

	if (isDead)
	{
		GetWorld()->GetTimerManager().SetTimer(AutoHideTimerHandle, this,
			&UNormalEnemy_HealthBarComponent::OnAutoHideTimerExpired, DeathHideDelay, false);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(AutoHideTimerHandle, this,
			&UNormalEnemy_HealthBarComponent::OnAutoHideTimerExpired, AutoHideDelay, false);
	}
}

// Hides the health bar when the auto-hide timer expires.
void UNormalEnemy_HealthBarComponent::OnAutoHideTimerExpired()
{
		SetVisibility(false);
}