// Fill out your copyright notice in the Description page of Project Settings.


#include "KwangTypes/KwangStructTypes.h"
#include "AbilitySystem/Abilities/KwangGameplayAbility.h"

bool FKwangHeroAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}