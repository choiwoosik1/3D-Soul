#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enemy/EnemyTypes.h"
#include "Enemy.generated.h"

// Enemy character class
UCLASS()
class PROJECT_SOUL_API AEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemy();

protected:
	// Combat stats
	UPROPERTY(EditAnywhere, Category = "Combat")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float BaseDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAllowBackstab = true;

	UPROPERTY(VisibleAnywhere,Category = "Combat")
	bool bBackstabbed = false;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bCriticalHit = false;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float BackstabMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float CriticalHitMultiplier = 3.f;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	AActor* AttackInitiator = nullptr;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<FAttackPattern> Patterns;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	int32 PatternIdx = -1;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	int32 AttackIdx = -1;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TArray<AActor*> AlreadyHitActors;
				
	// Poise system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poise")
	float MaxPoise = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise")
	float LimitPoise;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Poise")
	float CurrentPoise;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Poise")
	float LimitPoiseReductionRate = 0.2f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Poise")
	float PoiseThreshold = 0.4f;
	
	// Movement stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunSpeed = 400.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float StrafeSpeed = 125.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MinCombatRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxCombatRange = 500.f;
	
	// AI system
	UPROPERTY(EditAnywhere, Category = "AI")
	class UBehaviorTree* BTAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    EEnemyState CharacterState = EEnemyState::Idle;

	FTimerHandle DecisionTimerHandle;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	FPlayerActionRecord PlayerActionRecord;

	// Animation montages

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* CombatMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* StaggerMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* GroggyMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* BackstabMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* CriticalHitMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* DeathMontage = nullptr;

	virtual void BeginPlay() override;

	void SetCharacterState(EEnemyState NewState);

public:
	EEnemyState GetCharacterState() const { return CharacterState; }

	FPlayerActionRecord& GetPlayerActionRecord() { return PlayerActionRecord; }

	float GetCurrentHealth() const { return CurrentHealth; }

	float GetMaxHealth() const { return MaxHealth; }

	float GetMinCombatRange() const { return MinCombatRange; }

	float GetMaxCombatRange() const { return MaxCombatRange; }

	bool CanBeBackstabbed() const { return bAllowBackstab; }

	bool CanBeCriticalHit() const { return CharacterState == EEnemyState::Groggy; }

	virtual void SetSpeedByDistance(float Distance);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void StartDecisionTimer();

	void OnDecisionTimer();

	virtual void DecideNextAction();

	UFUNCTION(BlueprintCallable)
    void PerformAttackPattern(int32 AttackPatternIdx);

	UFUNCTION(BlueprintCallable)
	void FinishAttackPattern();

	UFUNCTION(BlueprintCallable)
	virtual void EnableWeaponHitbox();

	UFUNCTION(BlueprintCallable)
	virtual void DisableWeaponHitbox();

	UFUNCTION()
	void OnWeaponHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable)
	void EnableAttackRotation();

	UFUNCTION(BlueprintCallable)
	void DisableAttackRotation();

	UFUNCTION(BlueprintCallable)
	void EnableAttackMovement();

	UFUNCTION(BlueprintCallable)
	void DisableAttackMovement();

	void EnterPatrol();

	void EnterAlert();

	UFUNCTION(BlueprintCallable)
	virtual void EnterCombat();

	UFUNCTION(BlueprintCallable)
	virtual void ResumeCombat();

	void EnterStagger();

	UFUNCTION(BlueprintCallable)
	virtual void ExitStagger();

	void EnterGroggy();

	UFUNCTION(BlueprintCallable)
	virtual void ExitGroggy();

	void GetBackstabbed(AActor* Attacker);

	void GetCriticalHit(AActor* Attacker);

    virtual void Die();
};