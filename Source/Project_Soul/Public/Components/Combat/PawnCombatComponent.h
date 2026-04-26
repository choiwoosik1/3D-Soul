// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "GameplayTagContainer.h"
#include "PawnCombatComponent.generated.h"

class AKwangWeaponBase;
/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UPawnCombatComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()
	
public:
	// 1. [무기 등록 함수] 세상에 소환된 무기를 이 컴포넌트(인벤토리)에 등록함.
	// InWeaponTagToRegister: 무기의 이름표 (예: Weapon.Melee.GreatSword)
	// InWeaponToRegister: 실제로 소환된 무기 액터의 포인터 (우리가 만든 투명 타격 박스)
	// bRegisterAsEquippedWeapon: 등록과 동시에 바로 손에 쥘 것인지 여부
	UFUNCTION(BlueprintCallable, Category = "Kwang|Combat")	
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTagToRegister, AKwangWeaponBase* InWeaponToRegister, bool bRegisterAsEquippedWeapon = false);

	// 2. [무기 검색 함수] 태그(이름표)를 던져주면, 인벤토리(TMap)를 뒤져서 해당 무기 포인터를 찾아 돌려줌
	UFUNCTION(BlueprintCallable, Category = "Kwang|Combat")
	AKwangWeaponBase* GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	// 3. [현재 무기 태그] 캐릭터가 '지금' 손에 쥐고 활성화된 무기의 태그를 저장하는 변수
	UPROPERTY(BlueprintReadWrite, Category = "Kwang|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	// 4. [현재 무기 가져오기] 현재 들고 있는 무기 태그(CurrentEquippedWeaponTag)를 이용해, 실제 무기 액터 포인터를 바로 뽑아오는 간편 함수.
	UFUNCTION(BlueprintCallable, Category = "Kwang|Combat")
	AKwangWeaponBase* GetCharacterCurrentEquippedWeapon() const;

private:
	// 5. [비밀 인벤토리 주머니] 외부에서 함부로 못 건드리게 private으로 숨겨둔 '진짜 무기 보관함'.
	// 태그(Key)를 넣으면 무기 포인터(Value)가 나오는 TMap(딕셔너리) 구조
	TMap<FGameplayTag, AKwangWeaponBase*> CharacterCarriedWeaponMap;
};
