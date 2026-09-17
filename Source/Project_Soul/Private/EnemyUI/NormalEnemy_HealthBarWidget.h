#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NormalEnemy_HealthBarWidget.generated.h"

class UProgressBar;

UCLASS()
class PROJECT_SOUL_API UNormalEnemy_HealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HealthBar")
	void SetHealthPercent(float NewPercent);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthProgressBar;
};