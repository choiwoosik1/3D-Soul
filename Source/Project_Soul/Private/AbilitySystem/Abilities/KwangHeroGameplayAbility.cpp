// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/KwangHeroGameplayAbility.h"
#include "Characters/KwangHeroCharacters.h"
#include "KwangHeroController.h"

AKwangHeroCharacters* UKwangHeroGameplayAbility::GetHeroCharacterFromActorInfo()
{
    // 1. [캐시 확인] 이전에 찾아둔 영웅 캐릭터 주소(약한 포인터)가 아직 유효한지(살아있는지) 확인
    if (!CachedKwangHeroCharacter.IsValid())
    {
        // 2. [최초 탐색 & 저장] 비어있거나 객체가 파괴되었다면, 현재 스킬 시전자의 육체(AvatarActor)를 영웅 클래스로 변환(Cast)해서 캐시 변수에 저장
        CachedKwangHeroCharacter = Cast<AKwangHeroCharacters>(CurrentActorInfo->AvatarActor);
    }

    // 3. [안전 반환] 캐싱된 포인터가 유효하면 진짜 포인터(.Get())를 꺼내주고, 아니면 빈손(nullptr)을 반환하는 삼항 연산자.
    return CachedKwangHeroCharacter.IsValid() ? CachedKwangHeroCharacter.Get() : nullptr;
}

AKwangHeroController* UKwangHeroGameplayAbility::GetHeroControllerFromActorInfo()
{
    // 위와 완벽히 동일한 원리. 플레이어 컨트롤러를 찾아서 캐싱함.
    if (!CachedKwangHeroController.IsValid())
    {
        CachedKwangHeroController = Cast<AKwangHeroController>(CurrentActorInfo->PlayerController);
    }
    return CachedKwangHeroController.IsValid() ? CachedKwangHeroController.Get(): nullptr;
}

UHeroCombatComponent* UKwangHeroGameplayAbility::GetHeroCombatComponentFromActorInfo()
{
    // 1. [지름길 연계] 방금 위에서 만든 '영웅 캐릭터 가져오기' 함수를 먼저 실행함 (여기서 알아서 캐싱 처리까지 됨).
    // 2. [컴포넌트 반환] 그 캐릭터 본체 안에 이미 만들어져 있는 '영웅 전투 컴포넌트 꺼내기' 함수를 연달아 호출해서 바로 넘겨줌
    return GetHeroCharacterFromActorInfo()->GetHeroCombatComponent();
}
