// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Characters/KwangBaseCharacters.h"
#include "GameplayTagContainer.h"
#include "KwangHeroCharacters.generated.h"

// 전방 선언 (Forward Declaration): 컴파일 속도 향상을 위해 포인터 크기만 임시로 확보
class USpringArmComponent;
class UCameraComponent;
class UDataAsset_InputConfig;
struct FInputActionValue;
class UHeroCombatComponent;

UCLASS()
class PROJECT_SOUL_API AKwangHeroCharacters : public AKwangBaseCharacters
{
	GENERATED_BODY()

public:
	AKwangHeroCharacters();

protected:
	// [언리얼 특수 주석] 이 안쪽은 'APawn'이라는 조상 클래스가 물려준 기능들을 모아둔 곳이라고 표시
	// 나중에 언리얼 공식 문서처럼 예쁜 API 설명서를 자동 생성할 때 쓰이는 기능
	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interface

	// 1. 입력 바인딩 전용 함수: 플레이어가 이 캐릭터를 조종(Possess)하게 될 때 엔진이 자동으로 호출해 주는 함수
	// 이 안에서 "W키 누르면 앞으로 가라" 같은 케이블 연결 작업
	// override : 현재 클래스에서 새로 만든 게 아닌 부모 클래스 함수 덮어쓰기.
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// Called when the game starts or when spawned
	// virtual : 부모 클래스의 원본 함수가 있지만 현재 클래스 상황에 맞게 고침
	virtual void BeginPlay() override;
	
private:
// #pragma region: Visual Studio에서 코드를 접었다 펼칠 수 있게 해주는 단순 UI 기능
#pragma region Components

	// VisibleAnywhere: 에디터 디테일 패널에서 컴포넌트 속성을 볼 수 있게 함
	// BlueprintReadOnly: 블루프린트 노드에서 이 변수를 읽을 수만 있게 함 (덮어쓰기 방지)
	// meta=(AllowPrivateAccess="true"): C++에서는 private(외부 접근 불가)으로 안전하게 잠가두되, 에디터(블루프린트)에서는 예외적으로 접근할 수 있게 허용하는 옵션
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UHeroCombatComponent* HeroCombatComponent;

#pragma endregion

#pragma region Inputs
	// EditDefaultsOnly: 블루프린트 에디터 창에서 우리가 만든 '조작키 서류철(.asset)'을 꽂아 넣을 수 있게 해줌
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	UDataAsset_InputConfig* InputConfigDataAsset;

	// 3. 실제 이동 로직이 담길 콜백(Callback) 함수
	// 키보드(WASD)나 조이스틱을 움직였을 때 그 '입력된 값(앞으로 1, 옆으로 -1 등)'을 받아와서 캐릭터를 움직이게 함
	void Input_Move(const FInputActionValue& InputActionValue);

	void Input_Look(const FInputActionValue& InputActionValue);

	// 5. [스킬 버튼 누름 콜백] 유저가 공격이나 구르기 버튼을 '누르는 순간' 호출됨.
	// 입력 시스템이 "InputTag.Attack 눌렸어!" 하고 태그(InInputTag)를 여기로 던져주면, 여기서 GAS(뇌)에게 스킬 발동을 명령
	void Input_AbilityInputPressed(FGameplayTag InInputTag);

	// 6.[스킬 버튼 뗌 콜백] 유저가 누르고 있던 스킬 버튼에서 '손을 떼는 순간' 호출됨.
	// '차징 스킬(꾹 모았다가 쏘는 스킬)' 같은 걸 구현
	void Input_AbilityInputReleased(FGameplayTag InInputTag);
#pragma endregion

public:
	FORCEINLINE UHeroCombatComponent* GetHeroCombatComponent() const{ return HeroCombatComponent; }
};
