#pragma once

#include "CoreMinimal.h"
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
};

// Player action record for AI decision making
USTRUCT(BlueprintType)
struct FPlayerActionRecord
{
    GENERATED_BODY()

    float TimeSinceLastAttack;
    bool bIsBlocking;
    bool bIsRolling;
    float DistanceToEnemy;
};
