// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "HeroCombatComponent.generated.h"

class AKwangHeroWeapon;
class UParticleSystem;
class USoundBase;
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

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	AKwangHeroWeapon* GetHeroCurrentEquippedWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	float GetHeroCurrentEquippedWeaponDamageAtLevel(float InLevel) const;

	virtual void OnHitTargetActor(AActor* HitActor) override;
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor) override;
	// Impact effect spawned at the hit target on every melee hit (set in the Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit FX")
	TObjectPtr<UParticleSystem> HitImpactFX;

	// Impact sound played at the hit target on every melee hit (set in the Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit FX")
	TObjectPtr<USoundBase> HitImpactSound;
};
