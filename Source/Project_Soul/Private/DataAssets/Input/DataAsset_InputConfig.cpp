// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAssets/Input/DataAsset_InputConfig.h"

// 태그를 입력받아(InInputTag) 해당하는 액션을 반환하는 함수의 구현부
UInputAction* UDataAsset_InputConfig::FindNativeInputActionByTag(const FGameplayTag& InInputTag) const
{
	// 1. 범위 기반 for문 (Range-based for loop)
	// NativeInputActions 배열 안에 있는 아이템들을 처음부터 끝까지 순서대로 하나씩 꺼내서 'InputActionConfig'라는 임시 변수에 담아 검사
	// const & (상수 참조): 원본 데이터를 복사하지 않고 그대로 읽기만 하겠다는 뜻으로, 성능(메모리) 최적화를 위한 필수 C++ 문법
	for (const FKwangInputActionConfig& InputActionConfig : NativeInputActions)
	{
		// 2. 조건 검사
		// "현재 꺼내본 아이템의 태그가 내가 찾고 있는 태그(InInputTag)와 일치하는가?" AND
		// "그 아이템의 액션 부품이 비어있지 않고(null이 아니고) 정상적으로 들어있는가?"
		if (InputActionConfig.InputTag == InInputTag && InputActionConfig.InputAction)
		{
			// 조건을 만족하면, 배열 뒤쪽은 더 이상 찾을 필요 없이 그 액션 부품을 즉시 반환하고 함수를 종료
			return InputActionConfig.InputAction;
		}
	}

	// 3. 배열을 끝까지 다 뒤졌는데도 일치하는 태그를 못 찾았다면, 빈값(nullptr)을 반환
	return nullptr;
}
