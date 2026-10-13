#include "EnemyUI/NormalEnemy_HealthBarWidget.h"
#include "Components/ProgressBar.h"

// Sets the health percentage on the health bar progress bar.
void UNormalEnemy_HealthBarWidget::SetHealthPercent(float NewPercent)
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(FMath::Clamp(NewPercent, 0.f, 1.f));
	}
}