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

	// Share of roll catches assumed to land
	constexpr float RollCatchHitPrior = 0.5f;
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

// Why the current attack was started outside the normal decision flow
enum class EReactionAttack : uint8
{
	None,
	Punish,     // Answers the target's attack
	RollCatch   // Lands as the target's roll ends
};

inline const TCHAR* LexToString(EReactionAttack Type)
{
	switch (Type)
	{
	case EReactionAttack::Punish:    return TEXT("Punish");
	case EReactionAttack::RollCatch: return TEXT("RollCatch");
	default:                         return TEXT("None");
	}
}

// A moment where the enemy holds its attack and watches what the player does
struct FWaitObservation
{
	bool bActive = false;

	// Player attacked while the enemy was waiting
	bool bAttackedFirst = false;

	float EndTime = 0.f;
};

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

// What has been seen over the whole attack pattern currently running, for range learning
struct FPatternRun
{
	// Distance to the target when the pattern started
	float StartDistance = 0.f;

	// At least one swing touched the target: hit, guarded, or rolled through with invincibility
	bool bReached = false;

	// At least one swing was judged
	bool bResolved = false;

	// Every judged swing fell short without the target rolling
	bool bAllOutRanged = true;
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

	void Record(bool bPositive, float Weight = 1.f)
	{
		const float SampleDecay = FMath::Pow(BehaviorStats::Decay, Weight);
		Positive = Positive * SampleDecay + (bPositive ? Weight : 0.f);
		Total = Total * SampleDecay + Weight;
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

	// How often this pattern landed when used as a punish / as a roll catch
	UPROPERTY(VisibleAnywhere)
	FDecayedRate PunishHit;

	UPROPERTY(VisibleAnywhere)
	FDecayedRate CatchHit;

	void Record(EAttackOutcome Outcome, float Weight = 1.f)
	{
		const float SampleDecay = FMath::Pow(BehaviorStats::Decay, Weight);
		Hit = Hit * SampleDecay + (Outcome == EAttackOutcome::Hit ? Weight : 0.f);
		Dodged = Dodged * SampleDecay + (Outcome == EAttackOutcome::Dodged ? Weight : 0.f);
		Guarded = Guarded * SampleDecay + (Outcome == EAttackOutcome::Guarded ? Weight : 0.f);
		OutRanged = OutRanged * SampleDecay + (Outcome == EAttackOutcome::OutRanged ? Weight : 0.f);
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

	// How often an attack started as a roll catch landed its first swing
	UPROPERTY(VisibleAnywhere)
	FDecayedRate RollCatch;
};
