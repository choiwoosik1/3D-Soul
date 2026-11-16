#pragma once

#include "CoreMinimal.h"
#include "PlayerBehaviorStats.generated.h"

namespace BehaviorStats
{
	// Old observations are multiplied by this every time a new one arrives
	constexpr float Decay = 0.85f;

	// Number of imaginary samples at the prior rate; keeps the first few observations from dominating
	constexpr float PriorWeight = 2.f;

	// Rates assumed before anything is observed
	constexpr float HitPrior = 0.5f;
	constexpr float GuardPrior = 0.25f;
	constexpr float AggressionPrior = 0.5f;

	// Share of finished patterns the player is assumed to hit back after
	constexpr float PunishedPrior = 0.25f;

	// Share of punish attempts assumed to land
	constexpr float PunishHitPrior = 0.6f;
}

// Result of one swing against the player
enum class EAttackOutcome : uint8
{
	Hit,
	Dodged,     // Player rolled
	Guarded,
	OutRanged   // Swing never reached the player and the player did not roll
};

inline const TCHAR* LexToString(EAttackOutcome Outcome)
{
	switch (Outcome)
	{
	case EAttackOutcome::Hit:       return TEXT("Hit");
	case EAttackOutcome::Dodged:    return TEXT("Dodged");
	case EAttackOutcome::Guarded:   return TEXT("Guarded");
	default:                        return TEXT("OutRanged");
	}
}

// What has been seen of the player during the swing currently being judged
struct FSwingObservation
{
	// Hitbox is active and the swing will be recorded when it closes
	bool bOpen = false;

	// Hitbox touched the player
	bool bReached = false;

	// Player was guarding at contact, whether or not the guard held
	bool bGuarded = false;

	// Player rolled at some point since the previous swing ended
	bool bRolled = false;

	// The roll currently in progress has already been judged, either for this swing or for the previous one
	bool bRollHandled = false;

	// Outcome decided at contact; only meaningful when bReached
	EAttackOutcome ContactOutcome = EAttackOutcome::Hit;

	EAttackOutcome Resolve() const
	{
		if (bReached) return ContactOutcome;
		return bRolled ? EAttackOutcome::Dodged : EAttackOutcome::OutRanged;
	}
};

// A moment where the enemy holds its attack and watches what the player does
struct FWaitObservation
{
	bool bActive = false;

	// Player attacked while the enemy was waiting
	bool bAttackedFirst = false;

	float EndTime = 0.f;
};

// Decayed yes/no rate, e.g. "how often does the player guard"
USTRUCT()
struct FDecayedRate
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	float Positive = 0.f;

	UPROPERTY(VisibleAnywhere)
	float Total = 0.f;

	void Record(bool bPositive)
	{
		Positive = Positive * BehaviorStats::Decay + (bPositive ? 1.f : 0.f);
		Total = Total * BehaviorStats::Decay + 1.f;
	}

	// Returns PriorRate while there is no data, and moves toward the observed rate as samples arrive
	float Get(float PriorRate) const
	{
		return (Positive + BehaviorStats::PriorWeight * PriorRate) / (Total + BehaviorStats::PriorWeight);
	}
};

// What happened when one attack pattern was used against the player.
// One per FAttackPattern, and one more for punish attacks as a whole.
USTRUCT()
struct FPatternOutcomeStats
{
	GENERATED_BODY()

	// Decayed count of each swing outcome
	UPROPERTY(VisibleAnywhere)
	float Hit = 0.f;

	UPROPERTY(VisibleAnywhere)
	float Dodged = 0.f;

	UPROPERTY(VisibleAnywhere)
	float Guarded = 0.f;

	UPROPERTY(VisibleAnywhere)
	float OutRanged = 0.f;

	// How often the player hit back right after this pattern ended
	UPROPERTY(VisibleAnywhere)
	FDecayedRate Punished;

	// Learned multiplier on AttackRange; lowered when swings keep falling short
	UPROPERTY(VisibleAnywhere)
	float RangeScale = 1.f;

	// Measured seconds from attack start to the first hitbox activation (0 = not measured yet)
	UPROPERTY(VisibleAnywhere)
	float StartupTime = 0.f;

	void Record(EAttackOutcome Outcome)
	{
		Hit = Hit * BehaviorStats::Decay + (Outcome == EAttackOutcome::Hit ? 1.f : 0.f);
		Dodged = Dodged * BehaviorStats::Decay + (Outcome == EAttackOutcome::Dodged ? 1.f : 0.f);
		Guarded = Guarded * BehaviorStats::Decay + (Outcome == EAttackOutcome::Guarded ? 1.f : 0.f);
		OutRanged = OutRanged * BehaviorStats::Decay + (Outcome == EAttackOutcome::OutRanged ? 1.f : 0.f);
	}

	float GetHitRate(float PriorRate) const
	{
		const float Total = Hit + Dodged + Guarded + OutRanged;
		return (Hit + BehaviorStats::PriorWeight * PriorRate) / (Total + BehaviorStats::PriorWeight);
	}
};

// Player habits that do not belong to a single attack pattern.
// One per enemy instance, kept across respawns.
USTRUCT()
struct FPlayerBehaviorStats
{
	GENERATED_BODY()

	// How often the player guards a swing that reaches them
	UPROPERTY(VisibleAnywhere)
	FDecayedRate Guard;

	// How often the player attacks first while the enemy waits
	UPROPERTY(VisibleAnywhere)
	FDecayedRate Aggression;

	// How often an attack started as a punish landed its first swing
	UPROPERTY(VisibleAnywhere)
	FDecayedRate Punish;
};
