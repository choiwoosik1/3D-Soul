// Fill out your copyright notice in the Description page of Project Settings.


#include "KwangFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h"
#include "Interfaces/PawnCombatInterface.h"


UKwangAbilitySystemComponent* UKwangFunctionLibrary::NativeGetKwangASCFromActor(AActor* InActor)
{
    check(InActor);

    return Cast<UKwangAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));
}

void UKwangFunctionLibrary::AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
    UKwangAbilitySystemComponent* ASC = NativeGetKwangASCFromActor(InActor);

    if (!ASC->HasMatchingGameplayTag(TagToAdd))
    {
        ASC->AddLooseGameplayTag(TagToAdd);
    }
}

void UKwangFunctionLibrary::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
    UKwangAbilitySystemComponent* ASC = NativeGetKwangASCFromActor(InActor);

    if (!ASC->HasMatchingGameplayTag(TagToRemove))
    {
        ASC->RemoveLooseGameplayTag(TagToRemove);
    }
}

bool UKwangFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
    UKwangAbilitySystemComponent* ASC = NativeGetKwangASCFromActor(InActor);

    return ASC->HasMatchingGameplayTag(TagToCheck);
}

void UKwangFunctionLibrary::BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EKwangConfirmType& OutConfirmType)
{
    OutConfirmType = NativeDoesActorHaveTag(InActor, TagToCheck) ? EKwangConfirmType::Yes : EKwangConfirmType::No;
}

UPawnCombatComponent* UKwangFunctionLibrary::NativeGetPawnCombatComponentFromActor(AActor* InActor)
{
    check(InActor);

    if (IPawnCombatInterface* PawnCombatInterface = Cast<IPawnCombatInterface>(InActor))
    {
        return PawnCombatInterface->GetPawnCombatComponent();
    }


    return nullptr;
}

UPawnCombatComponent* UKwangFunctionLibrary::BP_GetPawnCombatComponentFromActor(AActor* InActor, EKwangValidTypes& OutValidType)
{
    UPawnCombatComponent* CombatComponent = NativeGetPawnCombatComponentFromActor(InActor);

    OutValidType = CombatComponent ? EKwangValidTypes::Valid : EKwangValidTypes::InValid;

    return CombatComponent;
}
