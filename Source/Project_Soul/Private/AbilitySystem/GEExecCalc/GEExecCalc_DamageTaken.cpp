// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GEExecCalc/GEExecCalc_DamageTaken.h"
#include "AbilitySystem/KwangAttributeSet.h"
#include "KwangGameplayTags.h"

#include "KwangDebugHelper.h"

// [데미지 계산에 필요한 어트리뷰트 캡처 정의 구조체]
// GAS의 ExecutionCalculation은 데미지 계산 시 필요한 어트리뷰트 값을
// 직접 참조하지 않고 '캡처(Snapshot)'해서 사용함
// Source = 공격자, Target = 피격자
struct FKwangDamageCapture
{
	// DECLARE_ATTRIBUTE_CAPTUREDEF: 캡처할 어트리뷰트의 '설계도(Def)'와 '임시 저장소(Property)'를 한 번에 선언
	// 내부적으로 AttackPowerProperty, AttackPowerDef 두 변수를 자동 생성해줌
    // DamageTaken: 최종 계산된 데미지를 실제로 적용할 '중간 통로' 어트리뷰트
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower)   // 공격자의 공격력
	DECLARE_ATTRIBUTE_CAPTUREDEF(DefensePower)  // 피격자의 방어력
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageTaken)   // 피격자의 DamageTaken 어트리뷰트 캡처 (Target = 피격자, false = 실시간 값 사용)

	FKwangDamageCapture()
	{
		// DEFINE_ATTRIBUTE_CAPTUREDEF(어트리뷰트셋, 어트리뷰트, Source/Target, bSnapshot)
		// bSnapshot = false: 계산 시점의 실시간 값을 사용 (true면 이펙트 적용 시점의 값을 고정해서 사용)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UKwangAttributeSet, AttackPower, Source, false)  // 공격자의 공격력 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UKwangAttributeSet, DefensePower, Target, false) // 피격자의 방어력 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UKwangAttributeSet, DamageTaken, Target, false)
	}
};

// [싱글턴 패턴] 캡처 구조체를 딱 한 번만 생성하고 재사용
// static 지역변수는 최초 호출 시 한 번만 생성되고 이후엔 같은 인스턴스를 반환함
// 매 계산마다 새로 생성하는 비용을 없애기 위한 최적화
static const FKwangDamageCapture& GetKwangDamageCapture()
{
	static FKwangDamageCapture KwangDamageCapture;
	return KwangDamageCapture;
}
UGEExecCalc_DamageTaken::UGEExecCalc_DamageTaken()
{
	/*Slow way of doing capture*/
	// FindFieldChecked로 리플렉션 시스템을 통해 프로퍼티를 직접 찾고
	// FGameplayEffectAttributeCaptureDefinition을 수동으로 만들어서 등록하는 방식
	// 매크로 방식보다 코드가 길고 실수하기 쉬워서 주석 처리됨
	//FProperty* AttackPowerProperty = FindFieldChecked<FProperty>(
	//	UKwangAttributeSet::StaticClass(),
	//	GET_MEMBER_NAME_CHECKED(UKwangAttributeSet, AttackPower)
	//);

	//FGameplayEffectAttributeCaptureDefinition AttackPowerCaptureDefinition(
	//	AttackPowerProperty,
	//	EGameplayEffectAttributeCaptureSource::Source,
	//	false
	//);

	//RelevantAttributesToCapture.Add(AttackPowerCaptureDefinition);*/

	RelevantAttributesToCapture.Add(GetKwangDamageCapture().AttackPowerDef);
	RelevantAttributesToCapture.Add(GetKwangDamageCapture().DefensePowerDef);
    RelevantAttributesToCapture.Add(GetKwangDamageCapture().DamageTakenDef);
}

void UGEExecCalc_DamageTaken::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    // [이펙트 스펙 가져오기] 현재 실행 중인 GameplayEffect의 모든 정보가 담긴 스펙을 꺼냄
    // 공격자 정보, 태그, SetByCaller 값 등이 전부 여기에 들어있음
    const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();

    // [컨텍스트 정보 접근] 이펙트를 발동시킨 주체들의 정보를 꺼낼 수 있음
    // 현재는 참조만 해두고 실제로 활용하진 않는 상태 (나중에 특수 조건 판정 시 활용 예정)
    EffectSpec.GetContext().GetSourceObject();  // 이펙트 소스 오브젝트 (무기 등)
    EffectSpec.GetContext().GetAbility();       // 이펙트를 발동시킨 어빌리티
    EffectSpec.GetContext().GetInstigator();    // 실제 공격을 지시한 주체 (플레이어 등)
    EffectSpec.GetContext().GetEffectCauser();  // 물리적으로 이펙트를 유발한 액터 (무기 액터 등)

    // [평가 파라미터 설정] 어트리뷰트 캡처 시 태그 조건을 걸 수 있는 필터 설정
    // 예: "출혈 태그가 있을 때만 추가 데미지" 같은 조건부 계산에 활용
    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags(); // 공격자 태그 목록
    EvaluateParameters.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags(); // 피격자 태그 목록

    // [공격력 캡처] 공격자의 AttackPower 어트리뷰트 실시간 값을 꺼내서 저장
    // AttemptCalculateCapturedAttributeMagnitude: 캡처된 어트리뷰트 값을 안전하게 읽어오는 함수
    // 캡처 실패 시 기본값(0.f) 유지
    float SourceAttackPower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetKwangDamageCapture().AttackPowerDef, EvaluateParameters, SourceAttackPower);
    Debug::Print(TEXT("SourceAttackPower"), SourceAttackPower);

    // [SetByCaller 값 파싱] 어빌리티에서 주입한 데미지 관련 수치들을 태그로 구분해서 꺼냄
    // SetByCallerTagMagnitudes: 태그-수치 쌍으로 저장된 Map을 순회
    float BaseDamage = 0.f;              // 무기 기본 데미지
    int32 UsedLightAttackComboCount = 0; // 현재 라이트 어택 콤보 카운트 (콤보에 따른 데미지 보정에 활용)
    int32 UsedHeavyAttackComboCount = 0; // 현재 헤비 어택 콤보 카운트 (현재 공격 1개지만 확장성을 위해 유지)

    for (const TPair<FGameplayTag, float>& TagMagnitude : EffectSpec.SetByCallerTagMagnitudes)
    {
        // 기본 데미지 태그와 일치하면 BaseDamage에 저장
        if (TagMagnitude.Key.MatchesTagExact(KwangGameplayTags::Shared_SetByCaller_BaseDamage))
        {
            BaseDamage = TagMagnitude.Value;
            Debug::Print(TEXT("BaseDamage"), BaseDamage);
        }

        // 라이트 어택 태그와 일치하면 콤보 카운트 저장 (콤보 수에 따라 데미지 배율 적용 예정)
        if (TagMagnitude.Key.MatchesTagExact(KwangGameplayTags::Player_SetByCaller_AttackType_Light))
        {
            UsedLightAttackComboCount = TagMagnitude.Value;
            Debug::Print(TEXT("UsedLightAttackComboCount"), UsedLightAttackComboCount);
        }

        // 헤비 어택 태그와 일치하면 콤보 카운트 저장
        if (TagMagnitude.Key.MatchesTagExact(KwangGameplayTags::Player_SetByCaller_AttackType_Heavy))
        {
            UsedHeavyAttackComboCount = TagMagnitude.Value;
            Debug::Print(TEXT("UsedHeavyAttackComboCount"), UsedHeavyAttackComboCount);
        }
    }

    // [방어력 캡처] 피격자의 DefensePower 어트리뷰트 실시간 값을 꺼내서 저장
    float TargetDefensePower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetKwangDamageCapture().DefensePowerDef, EvaluateParameters, TargetDefensePower);
    Debug::Print(TEXT("TargetDefensePower"), TargetDefensePower);

    // [라이트 어택 콤보 데미지 보정]
    // 콤보가 쌓일수록 데미지가 증가하는 구조
    // 1콤보: 1.0배, 2콤보: 1.05배, 3콤보: 1.10배 ...
    if (UsedLightAttackComboCount != 0)
    {
        const float DamageIncreasePercentLight = (UsedLightAttackComboCount - 1) * 0.05 + 1.f;

        BaseDamage *= DamageIncreasePercentLight;
        //Debug::Print(TEXT("ScaledBaseDamageLight"), BaseDamage);
    }

    // [헤비 어택 콤보 데미지 보정]
    // 라이트보다 콤보당 증가율이 높음 (0.15배씩)
    // 1콤보: 1.15배, 2콤보: 1.30배 ...
    if (UsedHeavyAttackComboCount != 0)
    {
        const float DamageIncreasePercentHeavy = UsedHeavyAttackComboCount * 0.15f + 1.f;

        BaseDamage *= DamageIncreasePercentHeavy;
        //Debug::Print(TEXT("ScaledBaseDamageHeavy"), BaseDamage);
    }

    // [최종 데미지 계산]
    // 공식: (기본데미지 * 공격력) / 방어력
    // 방어력이 높을수록 데미지가 줄어드는 나눗셈 구조
    const float FinalDamageDone = BaseDamage * SourceAttackPower / TargetDefensePower;

    if (FinalDamageDone > 0.f)
    {
        // [데미지 실제 적용]
        // DamageTakenProperty에 FinalDamageDone을 Override(덮어쓰기)로 설정
        // AttributeSet의 PostGameplayEffectExecute에서 이 값을 감지해
        // CurrentHealth -= DamageTaken 으로 실제 체력을 깎음
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(
                GetKwangDamageCapture().DamageTakenProperty, // 적용할 어트리뷰트
                EGameplayModOp::Override,                    // 덮어쓰기 방식
                FinalDamageDone                              // 최종 데미지 수치
            )
        );
    }
}
