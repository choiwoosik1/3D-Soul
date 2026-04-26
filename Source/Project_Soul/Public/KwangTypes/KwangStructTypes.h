// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "KwangStructTypes.generated.h"

class UKwangGameplayAbility;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct FKwangHeroAbilitySet
{
	GENERATED_BODY()

	// [입력 태그] 어떤 버튼을 눌렀을 때 발동할 것인가? (예: InputTag.Attack)
	// meta = (Categories = "InputTag")로 에디터에서 다른 엉뚱한 태그를 고르지 못하게 제한
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	// [부여할 스킬] 실제로 캐릭터에게 가르칠 스킬 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UKwangGameplayAbility> AbilityToGrant;

	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct FKwangHeroWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputMappingContext* WeaponInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FKwangHeroAbilitySet> DefaultWeaponAbilities;
};