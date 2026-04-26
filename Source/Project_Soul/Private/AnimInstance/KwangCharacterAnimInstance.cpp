// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/KwangCharacterAnimInstance.h"
#include "Characters/KwangBaseCharacters.h"
#include "GameFramework/CharacterMovementComponent.h"

void UKwangCharacterAnimInstance::NativeInitializeAnimation()
{
	// 1. 이 애니메이션을 덮어쓰고 있는 주인이 누구인지 찾아와 형변환
	OwningCharacter = Cast<AKwangBaseCharacters>(TryGetPawnOwner());

	// 2. 주인을 성공적으로 찾았다면 (포인터가 비어있지 않다면)
	if (OwningCharacter)
	{
		// 3. 주인의 무브먼트 컴포넌트(이동 관리자)도 찾아서 미리 캐싱
		OwningMovementComponent = OwningCharacter->GetCharacterMovement();
	}
}

void UKwangCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	// 1. 안전장치: 아직 주인이 스폰되지 않았거나, 이동 관리자를 못 찾았다면 계산을 중지하고 돌아감
	// 이 코드가 없으면 에디터를 켜자마자 (캐릭터가 맵에 생기기도 전에) 에러가 나면서 튕길 수 있음
	if (!OwningCharacter || !OwningMovementComponent)
	{
		return;
	}

	// 2. 현재 속력 계산 (GroundSpeed)
	// GetVelocity()로 현재 이동 방향과 속도(3D 벡터)를 가져온 뒤, Size2D()를 써서 Z축(위아래)은 무시하고 평면(X, Y)에서의 순수 이동 속력만 뽑아냄
	GroundSpeed = OwningCharacter->GetVelocity().Size2D();

	// 3. 가속 여부 계산 (bHasAcceleration)
	// GetCurrentAcceleration()으로 유저가 키보드(WASD)를 눌러서 힘을 가하고 있는지 확인합니다.
	// SizeSquared2D()가 0보다 크다는 것은 "조금이라도 힘이 들어가고 있다(true)"는 뜻
	bHasAcceleration = OwningMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;
}