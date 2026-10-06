// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_Skill_Base.h"
#include "AbilitySystemComponent.h"

UGA_Skill_Base::UGA_Skill_Base()
{
}

bool UGA_Skill_Base::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ManaCostEffectClasses)
	{
		return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (!ASC)
	{
		return false;
	}

	const FGameplayEffectSpecHandle CostSpec = MakeOutgoingGameplayEffectSpec(ManaCostEffectClasses, GetAbilityLevel());

	const bool bCanApplyCost = CostSpec.IsValid()
		&& ASC->CanApplyAttributeModifiers(ManaCostEffectClasses.GetDefaultObject(), GetAbilityLevel(), CostSpec.Data->GetContext());

	return bCanApplyCost && Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UGA_Skill_Base::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
}

const FGameplayTagContainer* UGA_Skill_Base::GetCooldownTags() const
{
	return nullptr;
}

void UGA_Skill_Base::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
}
