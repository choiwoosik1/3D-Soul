// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/KwangHeroGameplayAbility.h"
#include "HeroGameplayAbility_TargetLock.generated.h"

class UKwangWidgetBase;
/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UHeroGameplayAbility_TargetLock : public UKwangHeroGameplayAbility
{
	GENERATED_BODY()
	
protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility Interface

	UFUNCTION(BlueprintCallable)
	void OnTargetLockTick(float DeltaTime);


private:
	void TryLockOnTarget();
	void GetAvaillableActorsToLock();
	AActor* GetBestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);
	float CalculateThreatScore(const AActor* Candidate);
	void TryRetargetByThreat(float DeltaTime);
	bool TrySwitchToNextTarget();
	void GetNearbyActorsToLock();
	void DrawTargetLockAbility();
	void SetTargetLockWidgetPosition();

	void CancelTargetLockAbility();
	void CleanUp();

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	float BoxTraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	FVector TraceBoxSize = FVector(5000.f, 5000.f, 300.f);

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	TArray<TEnumAsByte<EObjectTypeQuery> > BoxTraceChannel;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	bool bShowPersistentDebugShape = false;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	TSubclassOf<UKwangWidgetBase> TargetLockWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	float TargetLockRotationInterpSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Camera")
	float MinLockPitch = -40.f;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Camera")
	float MaxLockPitch = 10.f;

	// Under this flat distance the camera pitch is blended toward ClosePitch
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Camera")
	float LockCloseDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Camera")
	float ClosePitch = -10.f;

	// Camera arm length while the target is very close
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Camera")
	float ClosePullBackLength = 320.f;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Camera")
	float ArmLengthInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	float MaxTargetLockDistance = 2500.f;

	// ---- Threat scoring weights (tunable in the GA Blueprint) ----
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float DistanceWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float AngleWeight = 1.0f;

	// Bonus when the enemy is currently attacking
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float AttackingWeight = 1.5f;

	// Extra bonus when the current attack can be parried
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float ParryableBonus = 0.5f;

	// While blocking, the attacking bonus is multiplied by this value
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float BlockingAttackMultiplier = 2.0f;

	// ---- Auto retarget while locked on ----
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	bool bAutoRetarget = true;

	// If true, the lock only switches while the player is blocking
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	bool bRetargetOnlyWhileBlocking = false;

	// How often the threat scores are re-evaluated (seconds)
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float RetargetInterval = 0.25f;

	// A new target must beat the current one by this score to take over
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float RetargetMargin = 1.0f;

	// Minimum time between two switches (seconds)
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float RetargetCooldown = 1.0f;

	// Search radius (all directions) for the next target when the locked one is lost
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock|Threat")
	float NextTargetSearchRadius = 1500.f;

	UPROPERTY()
	TArray<AActor*> AvailableActorsToLock;

	UPROPERTY()
	AActor* CurrentLockedActor;

	float DefaultArmLength = 0.f;

	UPROPERTY()
	UKwangWidgetBase* DrawnTargetLockWidget;

	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;

	float RetargetTimer = 0.f;
	float RetargetCooldownRemaining = 0.f;
};
