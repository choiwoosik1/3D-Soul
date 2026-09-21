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
	AActor* GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);
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

	UPROPERTY()
	TArray<AActor*> AvailableActorsToLock;

	UPROPERTY()
	AActor* CurrentLockedActor;

	UPROPERTY()
	UKwangWidgetBase* DrawnTargetLockWidget;

	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;
};
