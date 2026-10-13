// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/KwangHeroGameplayAbility.h"
#include "GA_Skill_Base.generated.h"

/**
 * 무사/궁수/주술사 모든 액티브 스킬의 공통 베이스.
 * 마나 소모(Cost)와 쿨다운을 GAS 표준 CheckCost/ApplyCost/GetCooldownTags/ApplyCooldown
 * 오버라이드로 통일 관리한다.
 */
UCLASS(abstract)
class PROJECT_SOUL_API UGA_Skill_Base : public UKwangHeroGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Skill_Base();

	//~ Begin UGameplayAbility Interface
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	//~ End UGameplayAbility Interface

protected:
	// UI 표시용 스킬 기본 정보
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Info")
	FText SkillDisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Info")
	FText SkillDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Info")
	TObjectPtr<UTexture2D> SkillIcon;

	// 마나 소모 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Cost")
	TSubclassOf<UGameplayEffect> ManaCostEffectClasses;

	// 쿨다운 지속시간을 관리하는 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|CoolDown")
	TSubclassOf<UGameplayEffect> SkillCooldownEffectClass;

	// 스킬 고유 쿨다운 식별 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Cooldown")
	FGameplayTag CooldownTag;

private:
	mutable FGameplayTagContainer CachedCooldownTags;
};
