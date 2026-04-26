// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KwangWeaponBase.generated.h"

class UBoxComponent;

UCLASS()
class PROJECT_SOUL_API AKwangWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKwangWeaponBase();

protected:

	// [무기의 외형 (눈에 보이는 부분)]
	// 3D 모델링(검, 창, 도끼 등)을 화면에 띄워줄 스태틱 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	UStaticMeshComponent* WeaponMesh;

	// [무기의 타격 판정 (안 보이는 센서)]
	// 칼날 주변을 감싸는 투명한 네모 박스입니다. 이 박스에 적이 닿으면 "때렸다!"라고 인식
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	UBoxComponent* WeaponCollisionBox;

public:
	// [인라인 Getter 함수]
	// 외부(캐릭터나 GAS)에서 이 무기의 타격 박스(WeaponCollisionBox) 정보를 달라고 할 때 꺼내주는 함수
	// FORCEINLINE을 붙여서 함수 호출 속도를 빠르게 만듦
	FORCEINLINE UBoxComponent* GetWeaponCollisionBox() const { return WeaponCollisionBox; }
};
