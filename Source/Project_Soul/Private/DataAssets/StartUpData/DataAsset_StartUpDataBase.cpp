// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/KwangGameplayAbility.h"

void UDataAsset_StartUpDataBase::GiveToAbilitySystemComponent(UKwangAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	// 1. [안전장치] ASC(캐릭터 뇌)가 정상적으로 존재하는지 검사. 비어있으면 여기서 즉시 크래시를 내서 개발자에게 오류를 알림
	check(InASCToGive);

	// 2. [기본 스킬 주입] 게임 시작 시 바로 켜져야 하는 스킬 배열을 실무 함수(GrantAbilities)로 넘김
	GrantAbilities(ActivateOnGivenAbilities, InASCToGive, ApplyLevel);

	// 3. [반응형 스킬 주입] 조건부로 발동되는 스킬(피격, 사망 등) 배열을 실무 함수로 넘김.
	GrantAbilities(ReactiveAbilities, InASCToGive, ApplyLevel);

}

void UDataAsset_StartUpDataBase::GrantAbilities(const TArray<TSubclassOf<UKwangGameplayAbility>>& InAbilitiesToGive, 
	UKwangAbilitySystemComponent* InKwangASCToGive, int32 ApplyLevel)
{
	// 1. [배열 검사] 에디터에서 실수로 스킬을 하나도 안 넣었을 경우, 헛수고하지 않고 바로 함수를 종료함
	if (InAbilitiesToGive.IsEmpty())
	{
		return;
	}

	// 2. [반복문] 배열 안에 든 스킬(Ability) 클래스들을 하나씩 꺼내서 확인
	for (const TSubclassOf<UKwangGameplayAbility>& Ability : InAbilitiesToGive)
	{
		// 3. [빈 칸 무시] 만약 배열에 None(빈 칸)이 섞여 있다면 뻗는 걸 막기 위해 무시하고 다음 칸으로 건너뜀
		if (!Ability) continue;

		// 4. [포장 작업] 단순 스킬 설계도(Class)를 GAS 전용 규격 봉투(FGameplayAbilitySpec)로 감싸서 객체화함.
		FGameplayAbilitySpec AbilitySpec(Ability);

		// 5. [출처 등록] 이 스킬의 주인이 누구인지 물리적 육체(AvatarActor)를 봉투에 기록함 (데미지 판정 등에 사용)
		AbilitySpec.SourceObject = InKwangASCToGive->GetAvatarActor();

		// 6. [레벨 세팅] 스킬의 초기 레벨을 봉투에 기록.
		AbilitySpec.Level = ApplyLevel;

		// 7. [최종 주입] 모든 세팅이 끝난 스킬 봉투(Spec)를 ASC 메모리에 최종적으로 꽂아 넣음
		InKwangASCToGive->GiveAbility(AbilitySpec);
	}
}
