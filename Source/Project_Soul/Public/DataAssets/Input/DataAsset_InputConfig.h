// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DataAsset_InputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

// 1. [태그(이름표) + 액션(실제 부품)]을 한 세트로 묶어주는 구조체 (가벼운 바구니)
// USTRUCT(BlueprintType): 이 구조체를 블루프린트에서도 변수 타입으로 쓸 수 있게 허용
USTRUCT(BlueprintType)
struct FKwangInputActionConfig
{
	GENERATED_BODY()

public:
	// 이 변수에 대해 이 입력 태그로 시작하는 태그만 선택할 수 있음.
	// meta = (Categories = "InputTag"): 언리얼 에디터에서 이 변수 값을 고를 때, 
	// 수많은 게임플레이 태그 중 반드시 'InputTag'라는 단어로 시작하는 태그들만 필터링해서 보여주도록 제한
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	// 태그와 짝지어질 실제 키보드/마우스 입력 부품
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputAction;

	// [안전 검사] 이 바구니 안에 '태그'와 '액션'이 둘 다 정상적으로 잘 들어있는지 확인하는 함수
	bool IsValid() const
	{
		return InputTag.IsValid() && InputAction;
	}
};

/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UDataAsset_InputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:

	// 2. 기본 입력 매핑 컨텍스트 (어떤 상황에서 어떤 키보드 맵을 쓸 것인가? - 예: 평상시 조작법 vs UI 열렸을 때 조작법)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputMappingContext* DefaultMappingContext;

	// 3. 위에서 만든 [태그+액션] 바구니(구조체)를 여러 개 담을 수 있는 '동적 배열(리스트)'
	// meta = (TitleProperty = "InputAction"): 에디터에서 배열을 펼쳤을 때 인덱스 번호([0], [1]) 대신 해당 액션의 이름으로 제목을 표시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FKwangInputActionConfig> NativeInputActions; 

	// 4. 태그(이름표)를 던져주면, 배열을 뒤져서 그 태그와 짝지어진 진짜 입력 액션(UInputAction)을 찾아주는 검색 함수
	UInputAction* FindNativeInputActionByTag(const FGameplayTag& InInputTag) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FKwangInputActionConfig> AbilityInputActions;
};
