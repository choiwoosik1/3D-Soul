// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "KwangTypes/KwangStructTypes.h"
#include "KwangAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UKwangAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void OnAbilityInputPressed(const FGameplayTag& InInputTag);
	void OnAbilityInputReleased(const FGameplayTag& InInputTag);
	
	UFUNCTION(BlueprintCallable, Category = "Kwang|Ability", meta = (ApplyLevel = "1"))
	void GrantHeroWeaponAbilities(const TArray<FKwangHeroAbilitySet>& InDefaultWeaponAbilities, 
		int32 ApplyLevel);
};
