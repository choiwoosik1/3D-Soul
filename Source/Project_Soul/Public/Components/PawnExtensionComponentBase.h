// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PawnExtensionComponentBase.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_SOUL_API UPawnExtensionComponentBase : public UActorComponent
{
	GENERATED_BODY()
	
protected:
	// 1. [템플릿 폰 가져오기] 이 부품(컴포넌트)을 달고 있는 진짜 주인(Pawn)을 원하는 클래스 타입(T)으로 한 번에 변환해서 가져오는 함수
	template <class T>
	T* GetOwningPawn() const
	{
		// [컴파일 방어막] 만약 프로그래머가 실수로 T 자리에 무기나 아이템 같은 '폰(APawn)이 아닌 클래스'를 넣으면, 
		// 게임을 실행하기도 전에 비주얼 스튜디오 빌드 단계에서 에러를 띄워서 원천 차단함.
		static_assert(TPointerIsConvertibleFromTo<T, APawn>::Value, "'T' Template Parameter to get GetPawn must be derived from APawn");

		// [강제 형변환] 주인이 무조건 존재하고, 무조건 T 타입일 것이라고 100% 확신할 때 쓰는 아주 빠르고 엄격한 캐스팅.
		return CastChecked<T>(GetOwner());
	}

	// 2. [기본 폰 가져오기] 굳이 특정 타입으로 변환할 필요 없이, 그냥 기본 APawn 형태로 주인을 가져올 때 쓰는 간편 함수
	APawn* GetOwningPawn() const
	{
		return GetOwningPawn<APawn>();
	}

	// 3. [템플릿 컨트롤러 가져오기] 주인을 조종하고 있는 '뇌(플레이어 컨트롤러 또는 AI 컨트롤러)'를 원하는 타입(T)으로 가져오는 함수.
	template <class T>
	T* GetOwningController() const
	{
		// [컴파일 방어막] T 자리에 컨트롤러(AController)가 아닌 클래스를 넣으면 빌드 단계에서 에러를 띄움.
		static_assert(TPointerIsConvertibleFromTo<T, AController>::Value, "'T' Template Parameter to GetController must be derived from AController");

		// 내 주인이 누구인지 찾고 -> 그 주인의 조종기를 뺏어와서 -> T 타입으로 변환해서 반환함
		return GetOwningPawn<APawn>()->GetController<T>();
	}

};
