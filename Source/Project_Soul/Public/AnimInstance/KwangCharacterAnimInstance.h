// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstance/KwangBaseAnimInstance.h"
#include "KwangCharacterAnimInstance.generated.h"

class AKwangBaseCharacters;
class UCharacterMovementComponent;
/**
 * 캐릭터의 애니메이션(뼈대 움직임)을 총괄하는 C++ 부모 클래스
 */
UCLASS()
class PROJECT_SOUL_API UKwangCharacterAnimInstance : public UKwangBaseAnimInstance
{
	GENERATED_BODY()

public:
	// 1. 초기화 함수: 게임이 시작될 때 (또는 캐릭터가 월드에 생성될 때) 딱 한 번 실행됨. 
	// 주로 캐릭터나 무브먼트 컴포넌트의 주소(포인터)를 미리 찾아두는 용도로 씀.
	virtual void NativeInitializeAnimation() override;

	// 2. 업데이트 함수: 매 프레임마다(Tick) 캐릭터의 현재 상태를 확인해서 변수를 갱신
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds);

protected:

	// 이 애니메이션을 입고 있는 진짜 '주인 캐릭터'의 포인터
	UPROPERTY()
	AKwangBaseCharacters* OwningCharacter;

	// 그 주인 캐릭터의 '이동 관리자' (현재 속도, 점프 여부 등을 여기서 빼옴.
	UPROPERTY()
	UCharacterMovementComponent* OwningMovementComponent;

	// 캐릭터가 땅에 붙어서 걷거나 뛰고 있는 '현재 속도'
	// BlueprintReadOnly: 애니메이션 블루프린트(노드 창)에서 이 값을 읽어갈 수 있게 허용함
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData ")
	float GroundSpeed;

	// 유저가 현재 키보드(WASD)나 패드로 '가속(이동 명령)'을 하고 있는지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData ")
	bool bHasAcceleration;
};
