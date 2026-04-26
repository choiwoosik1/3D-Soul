// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/KwangWeaponBase.h"
#include "Components/BoxComponent.h"

// Sets default values
AKwangWeaponBase::AKwangWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 1. [외형 만들기] 메모리에 'WeaponMesh'라는 이름으로 스태틱 메시 부품을 찍어냄
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));

	// 2. [뼈대 세우기] 방금 만든 메시를 이 무기 액터의 '가장 윗단(Root, 중심축)'으로 설정
	SetRootComponent(WeaponMesh);

	// 3. [센서 만들기] 메모리에 'WeaponCollisionBox'라는 이름으로 충돌 박스를 찍어냄
	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionBox"));

	// 4. [조립하기] 방금 만든 충돌 박스를 무기 외형(Root)에 자식으로 붙임.
	WeaponCollisionBox->SetupAttachment(GetRootComponent());

	// 5. [크기 조절] 박스의 크기(절반 길이)를 X, Y, Z 모두 20으로 설정
	WeaponCollisionBox->SetBoxExtent(FVector(20.f));

	// 6. [안전 장치] 태어날 때는 충돌 센서를 "꺼짐(NoCollision)" 상태로 만듦.
	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


