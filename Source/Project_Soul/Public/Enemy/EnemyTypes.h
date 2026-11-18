#pragma once

#include "CoreMinimal.h"
#include "Enemy/DodgeStats.h"
#include "Enemy/PlayerBehaviorStats.h"
#include "EnemyTypes.generated.h"

// Enumeration for enemy states, used for AI behavior and animation control
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
    Idle,
    Patrol,
    Alert,
    InCombat,
    Attack,
    Stagger,
    Groggy,
    Dead
};

// Enumeration for guard responses, used to determine how the player reacts to enemy attacks
UENUM(BlueprintType)
enum class EGuardResponse : uint8
{
    Block,       // Guard negates damage, consumes partial stamina
    GuardBreak,  // Guard negates damage but stamina drops to 0 and guard breaks
	Ignore       // Guard does not block the attack, player takes full damage
};

// Structure to define properties of an individual attack
USTRUCT(BlueprintType)
struct FAttackProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationSpeedR = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationSpeedL = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGuardResponse GuardResponse = EGuardResponse::Block;

	UPROPERTY(VisibleAnywhere)
	FDodgeStats DodgeStats;
};

// Structure to define an attack pattern, including animation montage and attack properties
USTRUCT(BlueprintType)
struct FAttackPattern
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAttackProperties> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange = 200.f;

	// How the player has dealt with this pattern (runtime only)
	UPROPERTY(VisibleAnywhere)
	FPatternOutcomeStats OutcomeStats;
};

// Player action record for AI decision making
USTRUCT(BlueprintType)
struct FPlayerActionRecord
{
	GENERATED_BODY()

	// Player habits that do not belong to a single pattern
	UPROPERTY(VisibleAnywhere)
	FPlayerBehaviorStats Behavior;

	// Attack pattern currently running
	FPatternRun Run;

	// Swing currently being judged
	FSwingObservation Swing;

	// Wait currently in progress
	FWaitObservation Wait;

	// The attack being executed was started as a punish
	bool bPunishAttack = false;

	// Whether the target was attacking at the previous decision; a punish is rolled only when an attack begins
	bool bTargetWasAttacking = false;

	// Dodge habit across all swings; supports swings that have few samples
	UPROPERTY(VisibleAnywhere)
	FDodgeStats DodgeFallback;

	// Dodge bucket of the previous lunge in the pattern currently being executed (INDEX_NONE at pattern start)
	int32 PrevDodgeBucket = INDEX_NONE;

	// Lunge attack currently being tracked
	FAttackSample PendingDodgeSample;
};
