// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/Hero/KwangHeroAnimInstance.h"
#include "Characters/KwangHeroCharacters.h"
#include "KwangGameplayTags.h"
#include "AbilitySystemComponent.h"

void UKwangHeroAnimInstance::NativeInitializeAnimation()
{
	// 1. 부모 클래스(CharacterAnimInstance)의 초기화 코드를 먼저 싹 실행해서 기본 세팅을 끝냄.
	Super::NativeInitializeAnimation();

	// 2. 부모가 찾아둔 'OwningCharacter(기본 캐릭터)'가 무사히 존재한다면?
	if (OwningCharacter)
	{
		// 3. 그 캐릭터가 우리 가문 사람(Hero)이 맞는지 다시 한번 정밀 검사(Cast)를 해서 전용 변수에 넣음
		OwningHeroCharacter = Cast<AKwangHeroCharacters>(OwningCharacter);
	}

	HitStunTag = FGameplayTag::RequestGameplayTag(FName("Player.Status.HitStun"), false);
}

void UKwangHeroAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	// 1. 부모 클래스의 업데이트를 먼저 실행(이때 속도나 가속도(bHasAcceleration)가 최신화됨)
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (OwningHeroCharacter == nullptr)
	{
		return; // 없으면 여기서 중단하고 다음 프레임에 다시 시도함
	}

	// 사망 상태면 업데이트 중단
	if (UAbilitySystemComponent* ASC = OwningHeroCharacter->GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(KwangGameplayTags::Shared_Status_Dead))
		{
			return;
		}
	}

	UAbilitySystemComponent* ASC = OwningHeroCharacter->GetAbilitySystemComponent();

	bool bIsAttacking = false;
	bool bIsHitStunned = false;
	if (ASC != nullptr)
	{
		// 가벼운 공격이나 무거운 공격 중 하나라도 수행 중인지 확인
		bIsAttacking = ASC->HasMatchingGameplayTag(KwangGameplayTags::Player_Ability_Attack_Light_Sword) ||
			ASC->HasMatchingGameplayTag(KwangGameplayTags::Player_Ability_Attack_Heavy_Sword);

		// 피격 경직 중인지 확인
		bIsHitStunned = HitStunTag.IsValid() && ASC->HasMatchingGameplayTag(HitStunTag);
	}

	// 2. 움직이거나 공격 중이거나 피격 경직 중이면
	if (bHasAcceleration || bIsAttacking || bIsHitStunned)
	{
		// 스톱워치(대기 시간)를 0초로 초기화
		IdleElpasedTime = 0.f;

		// 현재 움직이는 상태이니 RelaxMode를 false라고 블루프린트에 알립
		bShouldEnterRelaxState = false;
		bShouldEnterDeepBreathState = false;
	}
	else
	{
		IdleElpasedTime += DeltaSeconds;

		bShouldEnterRelaxState = (IdleElpasedTime >= EnterRelaxStateThreshold);
		bShouldEnterDeepBreathState = (IdleElpasedTime >= EnterDeepBreathStateThreshold);
	}
}

UKwangHeroAnimInstance* UKwangHeroAnimInstance::GetHeroAnimInstance()
{
	return this;
}
