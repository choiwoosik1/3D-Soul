// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h" // GAS의 기본 어빌리티 클래스
#include "KwangGameplayAbility.generated.h"

// [열거형(Enum)] 스킬의 '발동 방식'을 정의하는 메뉴판
// BlueprintType을 붙여서 블루프린트 에디터에서도 드롭다운 메뉴로 고를 수 있게 만듦

class UPawnCombatComponent;
class UKwangAbilitySystemComponent;

UENUM(BlueprintType)
enum class EKwangAbilityActivationPolicy : uint8
{
	OnTriggerd, // 1. 액티브 스킬 (유저가 마우스 좌클릭 등 '입력'을 했을 때 발동)
	OnGiven // 2. 패시브 스킬 (캐릭터가 이 스킬을 '부여(Give)'받자마자 즉시/자동으로 발동)
};
/**
 * 기본 UGameplayAbility를 상속받아, 커스텀 기능(발동 정책 등)을 추가한 베이스 클래스
 * 앞으로 만들 모든 스킬(평타, 방어, 구르기)은 이 클래스를 부모로 삼게된다.
 */
UCLASS()
class PROJECT_SOUL_API UKwangGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
protected:

	// 1. [스킬 부여 콜백] 캐릭터가 이 스킬을 '처음 배웠을 때(Given)' 딱 한 번 엔진이 호출해 주는 함수
	// 여기서 "어? 이 스킬 패시브(OnGiven)네? 그럼 배우자마자 바로 발동시켜!" 같은 로직
	//~ Begin UGameplayAbility Interface.
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	// 2.[스킬 종료 콜백] 스킬 사용이 무사히 끝났거나, 도중에 몬스터에게 맞아서 취소(Cancel)되었을 때 호출되는 함수
	// 여기서 "스킬 끝났으니 무적 판정(태그) 지워라!" 같은 뒷정리(청소) 로직을 짠다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility  Interface

	// 여기서 "스킬 끝났으니 무적 판정(태그) 지워라!" 같은 뒷정리(청소) 로직을 짠다.
	// EditDefaultsOnly: 블루프린트 원본(CDO)에서만 수정 가능하게 막아둠 (안전성 확보)
	// 기본값으로는 OnTriggerd(액티브 스킬)
	UPROPERTY(EditDefaultsOnly, Category = "KwangAbility")
	EKwangAbilityActivationPolicy AbilityActivationPolicy = EKwangAbilityActivationPolicy::OnTriggerd;

	// [전투 컴포넌트 호출 지름길]
	// BlueprintPure: 블루프린트에서 '실행 핀(하얀색 선)' 없이 데이터만 쏙 뽑아 쓰는 초록색 노드로 만들어줌.
	// const: 이 함수는 단순히 데이터를 읽어오기만 할 뿐, 내부 값을 함부로 바꾸지 않겠다는 선언
	UFUNCTION(BlueprintPure, Category = "Kwang|Ablity")
	UPawnCombatComponent* GetPawnCombatComponentFromActorInfo() const;

	// [커스텀 뇌(ASC) 호출 지름길]
	// 엔진 기본 ASC가 아닌, 우리가 직접 만든 'Kwang ASC'를 편하게 꺼내 쓰는 함수
	UFUNCTION(BlueprintPure, Category = "Kwang|Ablity")
	UKwangAbilitySystemComponent* GetKwangAbilitySystemComponentFromActorInfo() const;
};
