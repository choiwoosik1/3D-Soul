// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/KwangWeaponBase.h"
#include "Components/BoxComponent.h"
#include "KwangDebugHelper.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"        
#include "Characters/KwangHeroCharacters.h"

// Sets default values
AKwangWeaponBase::AKwangWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 1. [외형 만들기] 메모리에 'WeaponMesh'라는 이름으로 스태틱 메시 부품을 찍어냄
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));

	// 2. [뼈대 세우기] 방금 만든 메시를 이 무기 액터의 '가장 윗단(Root, 중심축)'으로 설정
	SetRootComponent(WeaponMesh);

	// 3. [센서 만들기] 메모리에 'WeaponCollisionBox'라는 이름으로 충돌 박스를 찍어냄
	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionBox"));

	// 4. [조립하기] 방금 만든 충돌 박스를 무기 외형(Root)에 자식으로 붙임.
	WeaponCollisionBox->SetupAttachment(GetRootComponent(), TEXT("WeaponSocket"));

	// 5. [크기 조절] 박스의 크기(절반 길이)를 X, Y, Z 모두 20으로 설정
	WeaponCollisionBox->SetBoxExtent(FVector(20.f));

	// 6. [안전 장치] 태어날 때는 충돌 센서를 "꺼짐(NoCollision)" 상태로 만듦.
	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponCollisionBox->SetHiddenInGame(false);
	WeaponCollisionBox->SetVisibility(true);

	WeaponCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxBeginOverlap);
	WeaponCollisionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxEndOverlap);
}

void AKwangWeaponBase::OnCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* WeaponOwingPawn = GetInstigator<APawn>();

	checkf(WeaponOwingPawn, TEXT("Forgot to assign an instigator as the owing pawn of the weapon : %s"), *GetName());

	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		if (WeaponOwingPawn != HitPawn)
		{
			/*Debug::Print(GetName() + TEXT("begin overlap with") + HitPawn->GetName(), FColor::Green);*/
		}
	}
}

void AKwangWeaponBase::OnCollisionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* WeaponOwingPawn = GetInstigator<APawn>();

	checkf(WeaponOwingPawn, TEXT("Forgot to assign an instigator as the owing pawn of the weapon : %s"), *GetName());

	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		if (WeaponOwingPawn != HitPawn)
		{
			/*Debug::Print(GetName() + TEXT("end overlap with") + HitPawn->GetName(), FColor::Red);*/
		}
	}
}

void AKwangWeaponBase::ToggleWeaponCollision(bool bEnable)
{
	bIsHitChecking = bEnable;

	//if (bEnable)
	//{
	//	AlreadyHitActors.Empty(); // 공격 시작 시 히트 리스트 초기화
	//}
}

void AKwangWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsHitChecking) 
	{
		ExecuteHitTrace();
	}
}

void AKwangWeaponBase::ExecuteHitTrace()
{
	// 1. 캐릭터 메시에서 직접 소켓 위치를 가져옴
	// 무기 에셋이 따로 없으니, 캐릭터가 쥐고 있는 칼의 시작과 끝 소켓 위치를 실시간으로 가져옴
	APawn* OwnerPawn = GetInstigator<APawn>();
	if (!OwnerPawn) return;

	AKwangHeroCharacters* OwnerCharacter = Cast<AKwangHeroCharacters>(OwnerPawn);
	if (!OwnerCharacter) return;

	USkeletalMeshComponent* CharMesh = OwnerCharacter->GetMesh();
	const FVector Start = CharMesh->GetSocketLocation(BladeStartSocketName);
	const FVector End = CharMesh->GetSocketLocation(BladeEndSocketName);

	// SphereTrace 코드 복구
	TArray<FHitResult> OutHits;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(GetOwner());


	// 2. 레이저(SphereTrace) 쏘기
	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(), Start, End, 30.0f,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false, ActorsToIgnore,
		EDrawDebugTrace::None,  // ← 변경
		OutHits, true,
		FLinearColor::Red,   // 히트 안됐을 때 색
		FLinearColor::Green, // 히트 됐을 때 색
		1.f                 // ← 잔상 유지 시간 (초) 원하는 만큼 조절
	);

	if (bHit)
	{
		for (const FHitResult& Hit : OutHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor)
			{
				if (APawn* HitPawn = Cast<APawn>(HitActor))
				{
					if (GetInstigator() != HitPawn)
					{
						UE_LOG(LogTemp, Warning, TEXT("ExecuteHitTrace HitActor: %s"), *HitActor->GetName());
						OnWeaponHitTarget.ExecuteIfBound(HitActor);
					}
				}
			}
		}
	}
}


