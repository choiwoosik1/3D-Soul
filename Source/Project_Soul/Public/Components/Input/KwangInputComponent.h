// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "KwangInputComponent.generated.h"

/**
 * 기본 입력 컴포넌트를 확장하여, '태그(Tag)'만으로 입력을 쉽게 연결할 수 있게 개조한 커스텀 입력 컴포넌트
 */
UCLASS()
class PROJECT_SOUL_API UKwangInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()
	
public:
	// C++ 템플릿(Template): 이 함수를 호출할 때 어떤 클래스(UserObject)의 어떤 함수(CallbackFunc)가 들어올지 모르니, 
	// 컴파일러가 알아서 타입을 맞춰달라고 열어두는 문법
	template<class UserObject, typename CallbackFunc>
	void BindNativeInputAction(const UDataAsset_InputConfig* InInputConfig, const FGameplayTag& InInputTag, 
		ETriggerEvent TriggerEvent, UserObject* ContextObject, CallbackFunc Func);

	template<class UserObject, typename CallbackFunc>
	void BindAbilityInputAction(const UDataAsset_InputConfig* InInputConfig, UserObject* ContextObject, 
		CallbackFunc InputPressedFunc, CallbackFunc InputReleasedFunc);
};

// 템플릿 함수는 헤더 파일(.h) 안에 구현부(몸통)까지 다 적어주어야 컴파일 에러가 나지 않는다. (인라인 구조)
template<class UserObject, typename CallbackFunc>
inline void UKwangInputComponent::BindNativeInputAction(const UDataAsset_InputConfig* InInputConfig, const FGameplayTag& InInputTag, ETriggerEvent TriggerEvent, UserObject* ContextObject, CallbackFunc Func)
{
	// 1. 안전장치 (Assertion)
	// InInputConfig(서류철)가 비어있으면(null) 뒤쪽 로직을 실행하지 않고 에디터를 그 자리에서 강제 종료(크래시)시킨다.
	// check(InInputConfig);
	checkf(InInputConfig, TEXT("Input Config Data Asset is null, can not proceed with binding"));

	// 2. 아까 만든 서류철 검색 함수 호출!
	// 서류철아, 이 태그(InInputTag)랑 짝지어진 실제 부품(InputAction) 좀 찾아와
	if (UInputAction* FoundAction = InInputConfig->FindNativeInputActionByTag(InInputTag))
	{
		// 3. 부품을 성공적으로 찾았다면, 부모 클래스(UEnhancedInputComponent)의 연결 함수를 실행.
		// 이 입력 부품(FoundAction)이 눌렸을 때(TriggerEvent), 이 객체(ContextObject)의 이 함수(Func)를 실행
		BindAction(FoundAction, TriggerEvent, ContextObject, Func);

	}
}

// [스킬 입력 자동 연결기 구현부]
template<class UserObject, typename CallbackFunc>
inline void UKwangInputComponent::BindAbilityInputAction(const UDataAsset_InputConfig* InInputConfig, UserObject* ContextObject, 
	CallbackFunc InputPressedFunc, CallbackFunc InputReleasedFunc)
{
	// 1. 데이터 애셋(서류철)이 있는지 확인
	checkf(InInputConfig, TEXT("Input Config Data Asset is null, can not proceed with binding"));

	// 2. [자동화 반복문] 서류철 안에 있는 '어빌리티 입력 배열(AbilityInputActions)'을 처음부터 끝까지 싹 다 훑음
	for (const FKwangInputActionConfig& AbilityInputActionConfig : InInputConfig->AbilityInputActions)
	{
		if (!AbilityInputActionConfig.IsValid()) continue;

		// 4. [눌렀을 때] 키보드를 누르는 순간(Started), 지정해 준 함수(InputPressedFunc)를 실행함.
		// 핵심: 이때 "어떤 태그(InputTag)의 버튼이 눌렸는지" 이름표를 함수에 같이 던져줌
		BindAction(AbilityInputActionConfig.InputAction, ETriggerEvent::Started, ContextObject, InputPressedFunc, AbilityInputActionConfig.InputTag);

		// 5. [뗐을 때] 키보드에서 손을 떼는 순간(Completed), 지정해 준 함수(InputReleasedFunc)를 실행함.
		BindAction(AbilityInputActionConfig.InputAction, ETriggerEvent::Completed, ContextObject, InputReleasedFunc, AbilityInputActionConfig.InputTag);
	}
}
