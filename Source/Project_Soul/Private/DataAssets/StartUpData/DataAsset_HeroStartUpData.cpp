// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "AbilitySystem/Abilities/KwangGameplayAbility.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h"

// 2. [스킬 주입 실행부] 데이터 애셋에 적힌 스킬들을 캐릭터의 뇌(ASC)에 밀어 넣는 작업
void UDataAsset_HeroStartUpData::GiveToAbilitySystemComponent(UKwangAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	// 부모 클래스가 하던 일(예: 패시브 스킬이나 기본 스탯 부여 등)을 먼저 뼈대로 실행
	Super::GiveToAbilitySystemComponent(InASCToGive, ApplyLevel);

	// [스킬+버튼 바구니] 배열을 처음부터 끝까지 싹 다 훑어봄
	for (const FKwangHeroAbilitySet& AbilitySet : HeroStartUpAbilitySets)
	{
		// 데이터가 불량이거나 비어있으면 쿨하게 무시하고 다음 스킬로 넘어감
		if (!AbilitySet.IsValid()) continue;

		// [스킬 명세서(Spec) 작성] GAS 시스템에게 제출할 '이 스킬 이렇게 등록해 주세요' 서류
		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);

		// 이 스킬을 쓰는 진짜 물리적 몸통이 누구인지 적어둠.
		AbilitySpec.SourceObject = InASCToGive->GetAvatarActor();

		// 스킬의 레벨 (1레벨 평타인지, 10레벨 평타인지)
		AbilitySpec.Level = ApplyLevel;

		// 스킬 명세서에다가 '입력 태그(예: InputTag.Attack)'를 붙여줌
		AbilitySpec.DynamicAbilityTags.AddTag(AbilitySet.InputTag);

		// 작성이 끝난 명세서를 캐릭터의 뇌(ASC)에게 제출해서 공식적으로 스킬을 가르침.
		InASCToGive->GiveAbility(AbilitySpec);
	}
}
