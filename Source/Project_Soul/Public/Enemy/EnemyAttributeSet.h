#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "EnemyAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECT_SOUL_API UEnemyAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UEnemyAttributeSet();

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FGameplayAttributeData DamageTaken;
    ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DamageTaken)
        
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FGameplayAttributeData AttackPower;
    ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, AttackPower)
    
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FGameplayAttributeData DefensePower;
    ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DefensePower)

    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};