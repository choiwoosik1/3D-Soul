// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/KwangAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/KwangGameplayAbility.h"

// 1. [스킬 발동 명령 하달] 육체(캐릭터)가 던진 태그(InInputTag)를 받아서 스킬을 쏘는 곳
void UKwangAbilitySystemComponent::OnAbilityInputPressed(const FGameplayTag& InInputTag)
{
	// 1. 들어온 태그가 비어있거나 불량이면 무시
	if (!InInputTag.IsValid())
	{
		return;
	}

	// 2. [뇌의 탐색] 내가 배운 모든 액티브 스킬 목록(GetActivatableAbilities)을 하나씩 쫙 꺼내서 검사
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		// 만약 이 스킬 이마에 붙은 바코드(DynamicAbilityTags)가 
		// 방금 눌린 버튼의 이름표(InInputTag)와 '정확히 일치하지 않는다면(!)' 패스하고 다음 스킬을 검사
		if (!AbilitySpec.DynamicAbilityTags.HasTagExact(InInputTag)) continue;

		// 3. [발사!] 오, 바코드가 똑같은 스킬을 찾았네? 
		// 그럼 이 스킬의 고유 번호표(Handle)를 넘겨주면서 당장 발동(TryActivateAbility)시켜라!
		TryActivateAbility(AbilitySpec.Handle);
	}
}

// 2. [스킬 종료/차징 해제 명령 하달]
void UKwangAbilitySystemComponent::OnAbilityInputReleased(const FGameplayTag& InInputTag)
{

}

void UKwangAbilitySystemComponent::GrantHeroWeaponAbilities(const TArray<FKwangHeroAbilitySet>& InDefaultWeaponAbilities, int32 ApplyLevel)
{
	if (InDefaultWeaponAbilities.IsEmpty())
	{
		return;
	}

	for (const FKwangHeroAbilitySet& AbilitySet : InDefaultWeaponAbilities)
	{
		if (!AbilitySet.IsValid()) continue;

		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);

		AbilitySpec.SourceObject = GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;
		AbilitySpec.DynamicAbilityTags.AddTag(AbilitySet.InputTag);

		GiveAbility(AbilitySpec);
	}
}
