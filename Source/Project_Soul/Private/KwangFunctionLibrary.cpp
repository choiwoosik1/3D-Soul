// Fill out your copyright notice in the Description page of Project Settings.


#include "KwangFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h"
#include "Interfaces/PawnCombatInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "KwangGameplayTags.h"

#include "KwangDebugHelper.h"


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

    if (ASC->HasMatchingGameplayTag(TagToRemove))
    {
        ASC->RemoveLooseGameplayTag(TagToRemove);
    }
}

bool UKwangFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
    UKwangAbilitySystemComponent* ASC = NativeGetKwangASCFromActor(InActor);

    if (!ASC) return false;

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

float UKwangFunctionLibrary::GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel)
{
    return InScalableFloat.GetValueAtLevel(InLevel);
}

FGameplayTag UKwangFunctionLibrary::ComputeHitReactDirectionTag(AActor* InAttacker, AActor* InVictim, float& OutAngleDifference)
{
    check(InAttacker && InVictim);

    const FVector VictimForward = InVictim->GetActorForwardVector();
    const FVector VictimToAttackerNormalized = (InAttacker->GetActorLocation() - InVictim->GetActorLocation()).GetSafeNormal();

    const float DotResult = FVector::DotProduct(VictimForward, VictimToAttackerNormalized);
    OutAngleDifference = UKismetMathLibrary::DegAcos(DotResult);

    const FVector CrossResult = FVector::CrossProduct(VictimForward, VictimToAttackerNormalized);

    if (CrossResult.Z < 0.f)
    {
        OutAngleDifference *= -1.f;
    }

    if (OutAngleDifference >= -45.f && OutAngleDifference <= 45.f)
    {
        return KwangGameplayTags::Shared_Status_HitReact_Front;
    }

    else if (OutAngleDifference < -45.f && OutAngleDifference >= -135.f)
    {
        return KwangGameplayTags::Shared_Status_HitReact_Left;
    }

    else if (OutAngleDifference < -135.f || OutAngleDifference > 135.f)
    {
        return KwangGameplayTags::Shared_Status_HitReact_Back;
    }

    else if (OutAngleDifference > 45.f && OutAngleDifference <= 135.f)
    {
        return KwangGameplayTags::Shared_Status_HitReact_Right;
    }

    return FGameplayTag();
}

bool UKwangFunctionLibrary::IsValidBlock(AActor* InAttacker, AActor* InDefender)
{
    check(InAttacker && InDefender);

    const float DotResult = FVector::DotProduct(InAttacker->GetActorForwardVector(), InDefender->GetActorForwardVector());

    const FString DebugString = FString::Printf(TEXT("Dot Result : %f %s"), DotResult, 
        DotResult < -0.1f ? TEXT("Valid Block") : TEXT("InValid Block"));

    Debug::Print(DebugString, DotResult > -0.1f ? FColor::Green : FColor::Red);

    return DotResult < -0.1f ? true : false;
}
