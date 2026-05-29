// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/KwangBaseAnimInstance.h"
#include "KwangFunctionLibrary.h"

bool UKwangBaseAnimInstance::DoesOwnerHaveTag(FGameplayTag TagToCheck) const
{
    if (APawn* OwningPawn = TryGetPawnOwner())
    {
        return UKwangFunctionLibrary::NativeDoesActorHaveTag(OwningPawn, TagToCheck);
    }
    return false;
}
