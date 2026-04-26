// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DataAsset_StartUpDataBase.generated.h"

class UKwangGameplayAbility;
class UKwangAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UDataAsset_StartUpDataBase : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// [메인 포장 뜯기 함수] 외부(캐릭터)에서 이 상자를 뜯을 때 호출할 함수
	virtual void GiveToAbilitySystemComponent(UKwangAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1);

protected:
	// 1. [즉시/기본 발동 스킬 배열]
	// 게임 시작하자마자 켜져야 하는 패시브나, 기본 평타 같은 어빌리티들의 목록(배열)
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray< TSubclassOf < UKwangGameplayAbility > > ActivateOnGivenAbilities;

	// 2. [조건부 반응 스킬 배열]
	// 평소엔 가만히 있다가, '맞았을 때(Hit React)', '죽었을 때(Dead React)' 등 특정 조건에서만 발동되는 어빌리티 목록
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray< TSubclassOf < UKwangGameplayAbility > > ReactiveAbilities;

	// [내부 작업용 도우미 함수]
	// 위에서 만든 배열들을 하나씩 빙글빙글 돌면서(for문), 진짜로 캐릭터의 뇌(ASC)에 스킬을 하나하나 부여해 주는(Grant) 실제 노동을 담당할 함수
	void GrantAbilities(const TArray< TSubclassOf < UKwangGameplayAbility > >& InAbilitiesToGive,
		UKwangAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1);
};
