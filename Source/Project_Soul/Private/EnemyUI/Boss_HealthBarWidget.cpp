#include "EnemyUI/Boss_HealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBoss_HealthBarWidget::SetBossName(const FText& NewBossName)
{
	if (BossNameText)
	{
		BossNameText->SetText(NewBossName);
	}
}

void UBoss_HealthBarWidget::SetHealthPercent(float Percent)
{
	if (BossHealthBar)
	{
		BossHealthBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}
