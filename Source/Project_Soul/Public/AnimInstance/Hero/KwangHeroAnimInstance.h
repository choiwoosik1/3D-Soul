// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstance/KwangCharacterAnimInstance.h"
#include "KwangHeroAnimInstance.generated.h"

class AKwangHeroCharacters;
/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UKwangHeroAnimInstance : public UKwangCharacterAnimInstance
{
	GENERATED_BODY()

public:
	// 1. 초기화 함수: 게임 시작 시 딱 한 번 실행됨.
	virtual void NativeInitializeAnimation() override;

	// 2. 업데이트 함수: 매 프레임마다(Tick) 애니메이션 변수를 갱신
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds);
	
protected:

	// [참조 변수] 부모 클래스의 OwningCharacter를 더 구체적인 '전우치(Hero)' 전용으로 캐스팅해서 들고 있을 변수
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|References")
	AKwangHeroCharacters* OwningHeroCharacter;

	// [상태 변수] "지금 릴렉스(대기) 상태로 들어가야 하는가?"를 블루프린트 노드(트랜지션)에 알려줄 스위치 역할
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	bool bShouldEnterRelaxState;

	// [상태 변수 2] 심호흡(Deep Breath) 전용 스위치 새로 추가
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	bool bShouldEnterDeepBreathState;

	// [설정값] "가만히 서서 몇 초가 지나야 릴렉스 모드로 들어갈 것인가?" (기본값: 5초)
	// EditDefaultsOnly 등을 쓰면 블루프린트에서 이 5초를 3초나 10초로 쉽게 바꿀 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float EnterRelaxStateThreshold = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float EnterDeepBreathStateThreshold = 7.f;

	// [내부 타이머] 실제로 캐릭터가 가만히 서 있던 시간을 누적해서 잴 스톱워치 변수
	// (블루프린트에서 알 필요가 없으므로 UPROPERTY를 안 붙인 것은 아주 훌륭한 설계
	float IdleElpasedTime;
};
