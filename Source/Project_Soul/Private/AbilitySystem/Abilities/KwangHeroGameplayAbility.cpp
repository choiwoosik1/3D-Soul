// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/KwangHeroGameplayAbility.h"
#include "Characters/KwangHeroCharacters.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h"
#include "KwangHeroController.h"
#include "KwangGameplayTags.h"

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

FGameplayEffectSpecHandle UKwangHeroGameplayAbility::MakeHeroDamageEffectSpecHandle(
    TSubclassOf<UGameplayEffect> EffectClass,   // 적용할 게임플레이 이펙트 클래스
    float InWeaponBaseDamage,                   // 무기 기본 데미지 수치
    FGameplayTag InCurrentAttackTypeTag,         // 현재 공격 타입 태그 (ex. LightAttack, HeavyAttack)
    int32 InUsedComboCount)                  // 현재 콤보 카운트
{
    // 1. [유효성 검사] 이펙트 클래스가 비어있으면 즉시 크래시 (필수 값이므로)
    check(EffectClass);

    // 2. [컨텍스트 생성] 이 이펙트가 '누가, 어디서, 무엇으로' 발생했는지 정보를 담는 컨텍스트 핸들 생성
    FGameplayEffectContextHandle ContextHandle = GetKwangAbilitySystemComponentFromActorInfo()->MakeEffectContext();

    // 3. [출처 정보 등록] 이 이펙트를 발동시킨 어빌리티, 소스 오브젝트, 시전자 정보를 컨텍스트에 등록
    ContextHandle.SetAbility(this);                                                              // 어빌리티 본체
    ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());                               // 소스 오브젝트 (무기 등)
    ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());   // 시전자 & 유발자

    // 4. [스펙 핸들 생성] 위에서 만든 컨텍스트를 바탕으로 실제 이펙트 스펙(데미지 계산용 설계도)을 생성
    FGameplayEffectSpecHandle EffectSpecHandle = GetKwangAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
        EffectClass,        // 어떤 이펙트인지
        GetAbilityLevel(),  // 어빌리티 레벨 (데미지 스케일링에 활용 가능)
        ContextHandle       // 위에서 만든 컨텍스트
    );

    // 5. [기본 데미지 설정] SetByCaller 방식으로 기본 데미지 수치를 이펙트 스펙에 주입
    //    (SetByCaller: 이펙트 내부에서 태그로 값을 받아 쓰는 방식, 런타임에 유연하게 수치 변경 가능)
    EffectSpecHandle.Data->SetSetByCallerMagnitude(
        KwangGameplayTags::Shared_SetByCaller_BaseDamage,
        InWeaponBaseDamage
    );

    // 6. [공격 타입별 콤보 수치 설정] 공격 타입 태그가 유효할 때만 콤보 카운트를 추가로 주입
    //    (ex. 라이트 어택 3콤보 → 데미지 보너스 계산에 활용)
    if (InCurrentAttackTypeTag.IsValid())
    {
        EffectSpecHandle.Data->SetSetByCallerMagnitude(InCurrentAttackTypeTag, InUsedComboCount);
    }

    // 7. [반환] 완성된 이펙트 스펙 핸들 반환 → 이걸 ApplyGameplayEffectSpecToTarget 등에 넘겨서 실제 데미지 적용
    return EffectSpecHandle;
}
