// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/KwangGameplayAbility.h"
#include "KwangHeroGameplayAbility.generated.h"

class AKwangHeroCharacters;
class AKwangHeroController;
/**
 * 기본 스킬(KwangGameplayAbility)을 상속받은 '영웅(플레이어) 전용' 스킬 베이스 클래스.
 * 몬스터용 스킬과 구분하여, 플레이어 조작에 특화된 기능들을 모아둔다.
 */
UCLASS()
class PROJECT_SOUL_API UKwangHeroGameplayAbility : public UKwangGameplayAbility
{
	GENERATED_BODY()
	
public:

	// 1. [영웅 육체] 스킬을 쓰고 있는 주체가 '영웅 캐릭터'임을 확신하고 바로 가져오는 함수.
	UFUNCTION(BlueprintPure, Category = "Kwang|Ablity")
	AKwangHeroCharacters* GetHeroCharacterFromActorInfo();

	// 2. [영웅 컨트롤러] 영웅을 조종하고 있는 '플레이어의 뇌(Player Controller)'를 가져오는 함수. UI나 입력 처리에 자주 쓰임
	UFUNCTION(BlueprintPure, Category = "Kwang|Ablity")
	AKwangHeroController* GetHeroControllerFromActorInfo();

	// 3. [영웅 전투 컴포넌트] 무기 장착, 콤보 등을 관리하는 영웅 전용 전투 컴포넌트를 가져옴
	UFUNCTION(BlueprintPure, Category = "Kwang|Ablity")
	UHeroCombatComponent* GetHeroCombatComponentFromActorInfo();

private:
	// [캐싱용 변수 - 약한 포인터(Weak Pointer)]
	// 매번 Cast 연산을 하면 느리니까, 처음 한 번 찾았을 때 여기다 저장(캐싱)해두고 다음부터는 이걸 바로 꺼내 쓰기 위한 변수
	TWeakObjectPtr<AKwangHeroCharacters> CachedKwangHeroCharacter;
	TWeakObjectPtr<AKwangHeroController> CachedKwangHeroController;
};
