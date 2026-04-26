// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/Hero/KwangHeroAnimInstance.h"
#include "Characters/KwangHeroCharacters.h"

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
}

void UKwangHeroAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	// 1. 부모 클래스의 업데이트를 먼저 실행(이때 속도나 가속도(bHasAcceleration)가 최신화됨)
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// 2. 만약 캐릭터가 움직이고 있다면? (가속도가 있다면)
	if (bHasAcceleration)
	{
		// 스톱워치(대기 시간)를 0초로 초기화
		IdleElpasedTime = 0.f;

		// 현재 움직이는 상태이니 RelaxMode를 false라고 블루프린트에 알립
		bShouldEnterRelaxState = false;
		bShouldEnterDeepBreathState = false;

	}
	else
	{
		// DeltaSeconds(1프레임당 걸린 시간, 보통 0.016초)를 스톱워치에 계속 더해줌
		// 즉, 현실 시간으로 스톱워치가 굴러감
		IdleElpasedTime += DeltaSeconds;

		// 누적된 스톱워치 시간이 5초(Threshold)를 넘었는지 검사
		// 5초를 넘겼다면 결과가 true가 되어 블루프린트에서 릴렉스 애니메이션으로 넘어감
		bShouldEnterRelaxState = (IdleElpasedTime >= EnterRelaxStateThreshold);
		bShouldEnterDeepBreathState = (IdleElpasedTime >= EnterDeepBreathStateThreshold);
	}
}