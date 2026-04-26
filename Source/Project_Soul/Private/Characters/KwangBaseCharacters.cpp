// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/KwangBaseCharacters.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h"
#include "AbilitySystem/KwangAttributeSet.h"

// Sets default values
AKwangBaseCharacters::AKwangBaseCharacters()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 액터가 게임 월드에 생성될 때 Tick 함수의 초기 활성화 상태 결정
	// Tick 기능을 아예 없앤 것은 아니고 게임 시작 시점에는 일시정지 상태
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 메시 컴포넌트(캐릭터 모델링)가 Scene에 배치된 Decal Material의 투영을 받을지 말지 결정
	// 월드에 배치되는 환경 그래픽 요소와 캐릭터 Mesh간의 Rendering 상호작용과 관련
	GetMesh()->bReceivesDecals = false;

	KwangAbilitySystemComponent = CreateDefaultSubobject<UKwangAbilitySystemComponent>(TEXT("KwangAbilitySystemComponent"));
	KwangAttributeSet = CreateDefaultSubobject<UKwangAttributeSet>(TEXT("KwangAttributeSet"));
}

UAbilitySystemComponent* AKwangBaseCharacters::GetAbilitySystemComponent() const
{
	return GetKwangAbilitySystemComponent();
}

void AKwangBaseCharacters::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (KwangAbilitySystemComponent)
	{
		KwangAbilitySystemComponent->InitAbilityActorInfo(this, this);

		ensureMsgf(!CharacterStartUpData.IsNull(), TEXT("Forgot to assign start up data to %s"), *GetName());

	}
}
