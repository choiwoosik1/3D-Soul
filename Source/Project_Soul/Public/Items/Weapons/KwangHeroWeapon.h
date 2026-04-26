// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/KwangWeaponBase.h"
#include "KwangTypes/KwangStructTypes.h"
#include "KwangHeroWeapon.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API AKwangHeroWeapon : public AKwangWeaponBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponData")
	FKwangHeroWeaponData HeroWeaponData;
};
