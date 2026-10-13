#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Boss_HealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class UBoss_HealthBarWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetBossName(const FText& NewBossName);
	void SetHealthPercent(float Percent);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;
};
