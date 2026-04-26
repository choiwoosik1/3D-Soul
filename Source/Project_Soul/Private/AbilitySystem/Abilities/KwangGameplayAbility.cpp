// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/KwangGameplayAbility.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h" // 뇌(ASC)에 직접 명령을 내리기 위해 포함
#include "Components/Combat/PawnCombatComponent.h"

// 1. [스킬 획득 시점]
void UKwangGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 부모 클래스의 기본 획득 로직을 먼저 실행해서 뼈대를 잡음
	Super::OnGiveAbility(ActorInfo, Spec);

	// 만약 방금 배운 이 스킬의 발동 정책이 '패시브(OnGiven, 배우자마자 자동 실행)'라면?
	if (AbilityActivationPolicy == EKwangAbilityActivationPolicy::OnGiven)
	{
		// ActorInfo(캐릭터 정보)가 무사히 존재하고 && 이 스킬이 아직 실행 중이 아니라면
		if (ActorInfo && !Spec.IsActive())

			// 캐릭터의 '뇌(AbilitySystemComponent)'에게 명령함: 
			// 이 스킬의 번호표(Spec.Handle) 줄 테니까, 지금 당장 이거 발동시켜(TryActivateAbility)
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

// 2. [스킬 종료 시점] 스킬 사용이 끝났거나 강제로 취소되었을 때
void UKwangGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 부모님의 기본 종료/청소 로직을 먼저 실행
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// 만약 이 스킬이 '패시브(OnGiven)' 였다면?
	if (AbilityActivationPolicy == EKwangAbilityActivationPolicy::OnGiven)
	{
		if (ActorInfo)
		{
			// 캐릭터의 뇌(ASC)에게 명령함:
			// 이 패시브 스킬 다 끝났으니까, 이제 지워버려(ClearAbility)
			ActorInfo->AbilitySystemComponent->ClearAbility(Handle);
		}
	}
}

// 3. [전투 컴포넌트 호출 지름길 구현부]
UPawnCombatComponent* UKwangGameplayAbility::GetPawnCombatComponentFromActorInfo() const
{
	// 1. [육체 찾기] GetAvatarActorFromActorInfo()로 현재 이 스킬을 시전하고 있는 물리적 몸통(캐릭터 액터)을 가져옴.
	// 2. [부품 검색] FindComponentByClass를 써서, 그 몸통에 붙어있는 수많은 부품 중 '전투 컴포넌트(UPawnCombatComponent)'만 딱 찾아서 반환
	return GetAvatarActorFromActorInfo()->FindComponentByClass<UPawnCombatComponent>();
}

// 4. [커스텀 뇌(ASC) 호출 지름길 구현부]
UKwangAbilitySystemComponent* UKwangGameplayAbility::GetKwangAbilitySystemComponentFromActorInfo() const
{
	// 1. [기본 뇌 가져오기] CurrentActorInfo(현재 스킬이 실행 중인 환경/캐릭터 정보) 안에 이미 저장되어 있는 기본 ASC 포인터를 빼옴.
	// 2. [형변환(Cast)] 그 기본 ASC를 우리가 직접 만든 'Kwang ASC' 규격으로 안전하게 강제 변환(Cast)해서 돌려줌
	return Cast<UKwangAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent);
}
