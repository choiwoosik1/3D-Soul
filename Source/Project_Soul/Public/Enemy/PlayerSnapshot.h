#pragma once

#include "CoreMinimal.h"

class AActor;

// What the player is doing at one moment, as seen by an enemy
enum class EPlayerAction : uint8
{
	Idle,
	Moving,
	Rolling,
	Attacking,
	Guarding
};

// Player state captured at one moment.
// Enemy code reads the player only through this struct.
struct PROJECT_SOUL_API FPlayerSnapshot
{
	EPlayerAction Action = EPlayerAction::Idle;
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;

	// Damage would not apply right now (roll invincibility frames, etc.)
	bool bInvincible = false;

	// False when Capture was called without a player
	bool bValid = false;

	static FPlayerSnapshot Capture(const AActor* Player);
};
