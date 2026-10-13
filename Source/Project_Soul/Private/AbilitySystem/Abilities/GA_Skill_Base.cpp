#include "AbilitySystem/Abilities/GA_Skill_Base.h"
#include "AbilitySystemComponent.h"

UGA_Skill_Base::UGA_Skill_Base()
{
}

bool UGA_Skill_Base::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (ManaCostEffectClasses)
	{
		UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (!ASC)
		{
			return false;
		}

		const UGameplayEffect* ManaCostEffect = ManaCostEffectClasses.GetDefaultObject();
		if (!ASC->CanApplyAttributeModifiers(ManaCostEffect, GetAbilityLevel(Handle, ActorInfo), MakeEffectContext(Handle, ActorInfo)))
		{
			return false;
		}
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UGA_Skill_Base::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (ManaCostEffectClasses)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, ManaCostEffectClasses.GetDefaultObject(), GetAbilityLevel(Handle, ActorInfo));
	}

	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

const FGameplayTagContainer* UGA_Skill_Base::GetCooldownTags() const
{
	CachedCooldownTags.Reset();

	if (const FGameplayTagContainer* ParentTags = Super::GetCooldownTags())
	{
		CachedCooldownTags.AppendTags(*ParentTags);
	}

	if (CooldownTag.IsValid())
	{
		CachedCooldownTags.AddTag(CooldownTag);
	}

	return &CachedCooldownTags;
}

void UGA_Skill_Base::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (SkillCooldownEffectClass && CooldownTag.IsValid())
	{
		const FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, SkillCooldownEffectClass, GetAbilityLevel(Handle, ActorInfo));

		if (CooldownSpec.IsValid())
		{
			CooldownSpec.Data->DynamicGrantedTags.AddTag(CooldownTag);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
		}
	}
}