// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/HeroCombatComponent.h"
#include "Items/Weapons/KwangHeroWeapon.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "KwangGameplayTags.h"
#include"KwangDebugHelper.h"
#include "Enemy/Enemy.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

AKwangHeroWeapon* UHeroCombatComponent::GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<AKwangHeroWeapon>(GetCharacterCarriedWeaponByTag(InWeaponTag));
}

AKwangHeroWeapon* UHeroCombatComponent::GetHeroCurrentEquippedWeapon() const
{
	return Cast<AKwangHeroWeapon>(GetCharacterCurrentEquippedWeapon());
}

float UHeroCombatComponent::GetHeroCurrentEquippedWeaponDamageAtLevel(float InLevel) const
{
	return GetHeroCurrentEquippedWeapon()->HeroWeaponData.WeaponBaseDamage.GetValueAtLevel(InLevel);
}

void UHeroCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	if (OverlappedActors.Contains(HitActor))
	{
		return;
	}

	OverlappedActors.AddUnique(HitActor);

	// 히트박스가 이미 HitActor를 들고 있어서 Enemy 쪽 LineTrace는 필요 없고,
	// 내 전방 벡터 vs 적 전방 벡터 각도만 비교하면 됨.
	// 각도 차이가 작다(<=15도) = 둘이 같은 방향을 보고 있다 = 내가 적 등 뒤에서 쳤다 → 백어택
	// 각도 차이가 크다(>=165도) = 서로 마주보고 있다 → 치명타 조건(그로기 등은 Enemy 쪽에서 체크)
	if (AEnemy* Enemy = Cast<AEnemy>(HitActor))
	{
		const float AngleDiff = FMath::RadiansToDegrees(
			FMath::Acos(FVector::DotProduct(
				GetOwningPawn()->GetActorForwardVector(),
				Enemy->GetActorForwardVector())));

		if (Enemy->CanBeBackstabbed() && AngleDiff <= 15.f)
		{
			Enemy->GetBackstabbed(GetOwningPawn());
		}
		else if (Enemy->CanBeCriticalHit() && AngleDiff >= 165.f)
		{
			Enemy->GetCriticalHit(GetOwningPawn());
		}
	}

	// Impact effect at the hit target, nudged toward the attacker so it is not buried inside the body
	if (HitImpactFX && GetOwningPawn())
	{
		const FVector ToOwner = (GetOwningPawn()->GetActorLocation() - HitActor->GetActorLocation()).GetSafeNormal2D();
		const FVector SpawnLocation = HitActor->GetActorLocation() + ToOwner * 40.f + FVector(0.f, 0.f, 30.f);
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitImpactFX, SpawnLocation, ToOwner.Rotation());
	}

	if (HitImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitImpactSound, HitActor->GetActorLocation());
	}

	FGameplayEventData Data;
	Data.Instigator = GetOwningPawn();
	Data.Target = HitActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		KwangGameplayTags::Shared_Event_MeleeHit,
		Data
	);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		KwangGameplayTags::Player_Event_HitPause,
		FGameplayEventData()
	);
}

void UHeroCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractedActor)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		KwangGameplayTags::Player_Event_HitPause,
		FGameplayEventData()
	);
}
