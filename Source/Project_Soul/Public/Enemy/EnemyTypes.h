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

USTRUCT(BlueprintType)
struct FDodgeRecord
{
    GENERATED_BODY();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector AttackDir = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector LastDodgeDirection = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector AccumulatedOffset = FVector::ZeroVector;
};

// Player action record for AI decision making
USTRUCT(BlueprintType)
struct FPlayerActionRecord
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FDodgeRecord DodgeRecord;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float TimeSinceLastAttack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsBlocking;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsRolling;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float DistanceToEnemy;

    void SetAttackDirection(FVector Dir)
    {
        DodgeRecord.AttackDir = Dir;
    }

    FVector GetAttackDirection()
    {
        return DodgeRecord.AttackDir;
    }

    FMatrix GetAttackTransform()
    {
        return FRotationMatrix(DodgeRecord.AttackDir.Rotation()).Inverse();
    }

    void RecordDodge(FVector DodgeDir)
    {
        DodgeRecord.LastDodgeDirection = DodgeDir;
        DodgeRecord.AccumulatedOffset = FMath::Lerp(DodgeRecord.AccumulatedOffset, DodgeDir, 0.5f);
    }

    FVector GetCorrectedOffset()
    {
        return DodgeRecord.AccumulatedOffset;
    }
};