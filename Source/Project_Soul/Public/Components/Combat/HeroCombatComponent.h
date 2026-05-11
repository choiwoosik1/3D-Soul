// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "HeroCombatComponent.generated.h"

class AKwangHeroWeapon;
/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UHeroCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	AKwangHeroWeapon* GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const;

	virtual void OnHitTargetActor(AActor* HitActor) override;
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor) override;
};
