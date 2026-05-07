// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/KwangEnemyGameplayAbility.h"
#include "Characters/EnemyCharacters.h"

// 어빌리티 정보로부터 적 캐릭터 객체를 찾아오고 캐싱함
AEnemyCharacters* UKwangEnemyGameplayAbility::GetEnemyCharactersFromActorInfo()
{
    // 이미 찾아둔 적 캐릭터 정보가 유효한지 확인함
    if (!CachedEnemyCharacters.IsValid())
    {
        // 정보가 없다면 현재 어빌리티를 실행 중인 아바타 액터를 적 캐릭터 타입으로 형변환하여 저장함
        CachedEnemyCharacters = Cast<AEnemyCharacters>(CurrentActorInfo->AvatarActor);
    }

    // 캐싱된 정보가 있으면 반환하고, 없으면 nullptr을 반환함
    return CachedEnemyCharacters.IsValid() ? CachedEnemyCharacters.Get() : nullptr;
}

// 적 캐릭터로부터 전투 컴포넌트(Combat Component)를 가져옴
UEnemyCombatComponent* UKwangEnemyGameplayAbility::GetEnemyCombatComponentFromActorInfo()
{
    // 위에서 만든 함수를 호출해 적 캐릭터를 먼저 찾은 뒤, 그 안에 있는 전투 컴포넌트를 반환함
    return GetEnemyCharactersFromActorInfo()->GetEnemyCombatComponent();
}
