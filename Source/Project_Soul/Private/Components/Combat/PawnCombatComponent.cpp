// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/PawnCombatComponent.h"
#include "Items/Weapons/KwangWeaponBase.h"

#include "KwangDebugHelper.h"

void UPawnCombatComponent::RegisterSpawnedWeapon(FGameplayTag InWeaponTagToRegister, AKwangWeaponBase* InWeaponToRegister, bool bRegisterAsEquippedWeapon)
{
	// 1. [안전 검사 1] 인벤토리(Map)에 이미 똑같은 이름표(Tag)를 가진 무기가 있는지 검사. 
	// 만약 있다면 즉시 크래시를 내고 "이 이름표 이미 쓰이고 있어!"라고 에러 메시지를 띄움
	checkf(!CharacterCarriedWeaponMap.Contains(InWeaponTagToRegister), 
		TEXT("A named named %s has already been added as carried weapon"), *InWeaponTagToRegister.ToString());

	// 2. [안전 검사 2] 등록하려는 무기 자체가 비어있는지(Null) 검사.
	check(InWeaponToRegister);

	// 3. [인벤토리 추가] Map에 태그(Key)와 무기 포인터(Value)를 짝지어서 쏙 집어넣음
	CharacterCarriedWeaponMap.Emplace(InWeaponTagToRegister, InWeaponToRegister);

	// 4. [즉시 장착 처리] 등록과 동시에 장착할 거냐고 물어봤다면(true), 현재 활성화된 무기 태그를 이걸로 덮어씌움
	if (bRegisterAsEquippedWeapon)
	{
		CurrentEquippedWeaponTag = InWeaponTagToRegister;
	}

	// 5. [디버그 로그] 무기가 성공적으로 등록되었다고 화면 왼쪽 위(또는 로그창)에 파란색/초록색 글씨로 띄워줌
	const FString WeaponString = FString::Printf(TEXT("A weapon named : %s has been registered using the tag %s"), 
		*InWeaponToRegister->GetName(), *InWeaponTagToRegister.ToString());
	Debug::Print(WeaponString);
}

AKwangWeaponBase* UPawnCombatComponent::GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
{
	// 1. [존재 여부 확인] 내가 찾으려는 태그가 인벤토리에 들어있긴 한지 먼저 물어봄
	if (CharacterCarriedWeaponMap.Contains(InWeaponTagToGet))
	{
		// 2. [무기 찾기] Find 함수로 Map을 뒤져서 무기가 있는 '메모리 주소의 주소(이중 포인터)'를 가져옴
		if (AKwangWeaponBase* const* FoundWeapon = CharacterCarriedWeaponMap.Find(InWeaponTagToGet))
		{
			// 3. [반환] 찾은 무기의 진짜 포인터를 꺼내서 넘겨줌
			return *FoundWeapon;
		}
	}

	// 4. [예외 처리] 인벤토리에 그런 태그가 없으면 "빈손(nullptr)"을 돌려줌.
	return nullptr;
}

AKwangWeaponBase* UPawnCombatComponent::GetCharacterCurrentEquippedWeapon() const
{
	// 1. [태그 검사] 현재 손에 쥐고 있다는 무기 태그가 아예 비어있거나 잘못된 태그인지 검사함
	if (!CurrentEquippedWeaponTag.IsValid())
	{
		return nullptr;
	}

	// 2. [검색 함수 재활용] 방금 위에서 만든 '태그로 무기 찾는 함수'에다가 현재 태그를 툭 던져서 나온 결과물을 바로 넘겨줌
	return GetCharacterCarriedWeaponByTag(CurrentEquippedWeaponTag);
}
