#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/BoxComponent.h"
#include "Enemy/EnemyTypes.h"
#include "Enemy.generated.h"

class AEnemy;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDied, AEnemy*, DeadEnemy);

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

	UPROPERTY(EditAnywhere, Category = "Combat")
	float BackstabMultiplier = 2.5f;
	
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bCanBeParried = false;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bCriticalHit = false;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float CriticalHitMultiplier = 3.f;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	bool bIsInvincible = false;

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

	// Weapon properties
	UPROPERTY(EditAnywhere, Category = "Weapon")
	UMeshComponent* RightWeaponMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	UBoxComponent* RightWeaponHitbox;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	FName RightWeaponSocketName = TEXT("HandGrip_R");

	UPROPERTY(EditAnywhere, Category = "Weapon")
	UMeshComponent* LeftWeaponMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	UBoxComponent* LeftWeaponHitbox;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	FName LeftWeaponSocketName = TEXT("HandGrip_L");
				
	// Poise system
	UPROPERTY(EditAnywhere, Category = "Poise")
	float MaxPoise = 30.f;

	UPROPERTY(VisibleAnywhere, Category = "Poise")
	float LimitPoise;
	
	UPROPERTY(VisibleAnywhere, Category = "Poise")
	float CurrentPoise;
	
	UPROPERTY(VisibleAnywhere, Category = "Poise")
	float LimitPoiseReductionRate = 0.2f;
	
	UPROPERTY(VisibleAnywhere, Category = "Poise")
	float PoiseThreshold = 0.4f;
	
	// Movement stats
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 150.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RunSpeed = 400.f;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float StrafeSpeed = 125.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float CloseToTarget = 40.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MinCombatRange = 150.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxCombatRange = 500.f;

	// How far MaxCombatRange may grow, as a multiple of its original value
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "1.0"))
	float MaxCombatRangeLimitScale = 1.3f;

	// AI system
	UPROPERTY(EditAnywhere, Category = "AI")
	class UBehaviorTree* BTAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    EEnemyState CharacterState = EEnemyState::Idle;

	FTimerHandle DecisionTimerHandle;

	// How often the enemy looks at the situation and decides what to do
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.05"))
	float DecisionInterval = 0.2f;

	// Shortest pause between the end of one pattern and the start of the next; punishes ignore it
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float MinAttackGap = 0.6f;

	float LastAttackEndTime = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	FPlayerActionRecord PlayerActionRecord;

	UPROPERTY(EditAnywhere, Category = "AI")
	bool bShowBehaviorDebug = false;

	// How strongly guard-beating patterns are favored against a player who guards a lot
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float GuardCounterBonus = 2.f;

	// Weight multiplier for the pattern that was used last
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RepeatPenalty = 0.5f;

	// Chance to hold an attack when the player shows no tendency either way (0 = never wait)
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float BaseWaitChance = 0.5f;

	// How long one wait lasts; a random length in this range is picked each time
	UPROPERTY(EditAnywhere, Category = "AI")
	FFloatInterval WaitDuration = FFloatInterval(0.4f, 1.8f);

	// A pattern can be used as a punish if its startup is within this many seconds of the enemy's fastest pattern
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float PunishStartupMargin = 0.2f;

	// How strongly patterns the player hits back after are avoided
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float PunishedPenalty = 1.f;

	int32 PunishWatchIdx = INDEX_NONE;

	FTimerHandle PunishWatchTimerHandle;

	// Time after a punish during which attacks are not answered; a random length in this range is picked each time
	UPROPERTY(EditAnywhere, Category = "AI")
	FFloatInterval PunishCooldown = FFloatInterval(2.f, 4.f);

	float NextPunishTime = 0.f;

	// How far ahead along the target's roll the landing point is predicted, in seconds
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float RollLookAhead = 0.5f;

	// Seconds after the target's roll starts at which a roll catch should open its first hitbox
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float RollCatchLandTime = 0.8f;

	// How much later than the landing time a pattern may still arrive to be used as a roll catch
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float RollCatchTolerance = 0.2f;

	// Time after a roll catch during which rolls are not answered; a random length in this range is picked each time
	UPROPERTY(EditAnywhere, Category = "AI")
	FFloatInterval RollCatchCooldown = FFloatInterval(3.f, 6.f);

	float NextRollCatchTime = 0.f;

	FTimerHandle RollCatchTimerHandle;

	int32 PendingCatchIdx = INDEX_NONE;

	float PendingCatchDistance = 0.f;

	float AttackStartTime = 0.f;

	FTimerHandle SwingObserveTimerHandle;

	// Chance to apply aim offset to attacks
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxAimShiftChance = 1.f;

	// Maximum distance the enemy can aim off-target when applying aim offset
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float MaxAimShift = 600.f;

	// Highest learned range multiplier for patterns that move the enemy during the attack
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "1.0"))
	float MaxRangeScaleMoving = 1.5f;

	// Highest learned range multiplier for patterns that attack in place
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "1.0"))
	float MaxRangeScaleStatic = 1.1f;

	// Share of each learned sample that same-class enemies receive (0 = learn alone)
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LearningShareRatio = 0.3f;

	TArray<TWeakObjectPtr<AEnemy>> Peers;

	bool bPeersCached = false;

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
	UAnimMontage* ParriedMontage = nullptr;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* DeathMontage = nullptr;

	FTransform InitialTransform;

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	void SetCharacterState(EEnemyState NewState);

	UFUNCTION(BlueprintCallable)
	void ExitStagger();

	UFUNCTION(BlueprintCallable)
	void ExitGroggy();

	virtual void ResumeCombat();

	void DisableInvincible() { bIsInvincible = false; }

	virtual void NotifyHealthBarUpdate(float NewCurrentHealth, float NewMaxHealth) {}

	virtual void StartDecisionTimer();

	void OnDecisionTimer();

	AActor* GetCombatTarget() const;

	float GetReactionChance(EReactionAttack Type) const;

	bool StartReactionAttack(EReactionAttack Type, float Distance);

	int32 PickPattern(float Distance, EReactionAttack Reaction = EReactionAttack::None) const;

	virtual float ScorePattern(int32 Idx) const;

	virtual float GetWaitChance() const;

	void DrawBehaviorDebug();

	void StartPendingCatch();

	void BeginReactionAttack(int32 Idx, EReactionAttack Type, float Distance);

	void PerformAttackPattern(int32 AttackPatternIdx);

	void ObserveSwing();

	UFUNCTION(BlueprintCallable)
	void EnableAttackRotation();

	UFUNCTION(BlueprintCallable)
	void DisableAttackRotation();

	UFUNCTION(BlueprintCallable)
	void EnableAttackMovement();

	FVector ApplyAimOffset(const FVector& BaseLocation, const FVector& Forward, const FAttackProperties& Attack, int32 PrevBucket) const;

	UFUNCTION(BlueprintCallable)
	void DisableAttackMovement();

	UFUNCTION(BlueprintCallable)
	void SetParryable(bool bIsParryable) { bCanBeParried = bIsParryable; }

	UFUNCTION(BlueprintCallable)
	virtual void EnableWeaponHitbox();

	void TryWeaponHit(AActor* OtherActor);

	UFUNCTION(BlueprintCallable)
	virtual void DisableWeaponHitbox();

	int32 RecordSample(const FAttackSample& Sample);

	void InterruptCurrentAttack();

	UFUNCTION(BlueprintCallable)
	void FinishAttackPattern();

	void CloseWait();

	void CloseReactionAttempt(bool bHit);

	void ClosePunishWatch(bool bPunished);

	void UpdateRangeScale(int32 Idx);

	void ShareLearning(TFunctionRef<void(AEnemy& Enemy, float Weight)> Apply);

	const TArray<TWeakObjectPtr<AEnemy>>& GetPeers();

	virtual void Die();

	void ResetDecisionState();

public:
	UPROPERTY(BlueprintAssignable)
	FOnEnemyDied OnEnemyDied;

	EEnemyState GetCharacterState() const { return CharacterState; }

	float GetCurrentHealth() const { return CurrentHealth; }

	float GetMaxHealth() const { return MaxHealth; }

	bool CanBeBackstabbed() const { return bAllowBackstab && !bIsInvincible; }

	bool CanBeCriticalHit() const { return CharacterState == EEnemyState::Groggy && !bIsInvincible; }

	bool CanBeParried() const { return bCanBeParried; }

	float GetMinCombatRange() const { return MinCombatRange; }

	float GetMaxCombatRange() const;

	void UpdateBlackboardCombatRange();

	virtual void SetSpeedByDistance(float Distance);

	virtual void NotifyHealthBarLockOn(bool bIsLockedOn) {}

	void EnterPatrol();

	void EnterAlert();

	void EnterCombat();

	void EnterStagger();

	void EnterGroggy();

	void GetBackstabbed(AActor* Attacker);

	void GetCriticalHit(AActor* Attacker);

	void GetParried(AActor* Attacker);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
		class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void DecideNextAction();

	float GetEffectiveRange(int32 Idx) const;

	UFUNCTION()
	void OnWeaponHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {	TryWeaponHit(OtherActor); }

	virtual void ResetToInitialState();
};
