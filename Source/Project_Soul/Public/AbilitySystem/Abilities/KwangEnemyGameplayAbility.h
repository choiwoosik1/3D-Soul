// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/KwangGameplayAbility.h"
#include "KwangEnemyGameplayAbility.generated.h"

class AEnemyCharacters;
class UEnemyCombatComponent;
/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UKwangEnemyGameplayAbility : public UKwangGameplayAbility
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Kwang|Ability")
	AEnemyCharacters* GetEnemyCharactersFromActorInfo();

	UFUNCTION(BlueprintPure, Category = "Kwang|Ability")
	UEnemyCombatComponent* GetEnemyCombatComponentFromActorInfo();

private:
	TWeakObjectPtr<AEnemyCharacters> CachedEnemyCharacters;
};
