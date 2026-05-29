// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "KwangBaseAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UKwangBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool DoesOwnerHaveTag(FGameplayTag TagToCheck) const;
};
