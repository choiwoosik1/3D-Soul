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

// Enumeration for guard responses, used to determine how the player reacts to enemy attacks
UENUM(BlueprintType)
enum class EGuardResponse : uint8
{
    Block,       // Guard negates damage, consumes partial stamina
    GuardBreak,  // Guard negates damage but stamina drops to 0 and guard breaks
	Ignore       // Guard does not block the attack, player takes full damage
};

// Dodge direction relative to the attack axis (Forward = enemy -> target at commit time)
enum class EDodgeBucket : uint8
{
	Stay,
	Left,
	Right,
	Away,
	Toward
};

// Decayed observation weight and mean offset (Forward, Right) of one dodge bucket
USTRUCT()
struct FDodgeBucketStat
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	float Weight = 0.f;

	UPROPERTY(VisibleAnywhere)
	FVector2D MeanOffset = FVector2D::ZeroVector;
};

// Learned dodge habit of the target.
// The next dodge direction is predicted from a previous dodge supplied by the caller
// (previous swing of the same pattern for melee, previous shot for ranged),
// and its distance from the mean offset of that direction.
USTRUCT()
struct FDodgeStats
{
	GENERATED_BODY()

	static constexpr int32 BucketCount = 5;

	// Weight multiplier applied to every bucket on each new sample (recent habits count more)
	static constexpr float Decay = 0.8f;

	// Offsets shorter than this (cm) count as "target stayed"
	static constexpr float StayDistance = 80.f;

	// Lateral share needed to classify as Left/Right: 0.5 = within about 63 degrees of the side,
	// so diagonal rolls count as sideways and only near-straight dodges become Away/Toward
	static constexpr float LateralBias = 0.5f;

	// Weight of the fallback stats relative to the swing's own stats
	static constexpr float FallbackScale = 0.5f;

	// Higher value = more evidence needed before confidence grows
	static constexpr float ConfidenceK = 2.f;

	// Minimum bucket weight (about two observations) before any correction is returned
	static constexpr float MinWeight = 2.f;

	// Minimum share of the predicted direction; filters out players with no clear pattern
	static constexpr float MinShare = 0.6f;

	// Stricter share for a dodge with no previous one in the sequence (the first lunge of a combo):
	// which pattern the player opens with is a free choice, so only a clear habit is corrected
	static constexpr float OpenerMinShare = 0.75f;

	// Per-direction weight and mean offset, indexed by EDodgeBucket (lazily sized on the first Record())
	UPROPERTY(VisibleAnywhere)
	TArray<FDodgeBucketStat> Buckets;

	// Transition weights, row-major [Previous * BucketCount + Next]
	UPROPERTY(Transient, VisibleAnywhere)
	TArray<float> Transitions;

	static bool IsValidBucket(int32 Bucket) { return Bucket >= 0 && Bucket < BucketCount; }

	static EDodgeBucket Classify(const FVector2D& Offset)
	{
		if (Offset.Size() < StayDistance) return EDodgeBucket::Stay;

		if (FMath::Abs(Offset.Y) >= FMath::Abs(Offset.X) * LateralBias)
		{
			return Offset.Y > 0.f ? EDodgeBucket::Right : EDodgeBucket::Left;
		}
		return Offset.X > 0.f ? EDodgeBucket::Away : EDodgeBucket::Toward;
	}

	// Add one observation: the target's displacement (Forward, Right) between aim commit and attack resolution.
	// PrevBucket is the dodge this one followed (INDEX_NONE if none). Returns the bucket of this observation.
	int32 Record(const FVector2D& Offset, int32 PrevBucket)
	{
		if (Buckets.Num() != BucketCount) Buckets.SetNum(BucketCount);
		if (Transitions.Num() != BucketCount * BucketCount) Transitions.SetNumZeroed(BucketCount * BucketCount);

		const int32 Bucket = static_cast<int32>(Classify(Offset));

		// Direction stats: dodge distance of each direction, and the prediction when there is no previous dodge
		for (FDodgeBucketStat& Stat : Buckets) Stat.Weight *= Decay;
		FDodgeBucketStat& Stat = Buckets[Bucket];
		Stat.Weight += 1.f;

		// Weighted running mean: the new sample has weight 1 against the (already decayed) accumulated weight
		Stat.MeanOffset += (Offset - Stat.MeanOffset) / Stat.Weight;

		// Transition stats: only the row of the previous dodge learns what followed it
		if (IsValidBucket(PrevBucket))
		{
			float* Row = &Transitions[PrevBucket * BucketCount];
			for (int32 i = 0; i < BucketCount; ++i) Row[i] *= Decay;
			Row[Bucket] += 1.f;
		}
		return Bucket;
	}

	// Predict the next dodge offset (Forward, Right), merging this swing's stats with the fallback.
	// Uses the transition row of PrevBucket, or the plain direction frequency when there is no previous dodge.
	// Returns false when the pattern is not clear yet, or when the target is predicted to stay on the attack line.
	bool Query(const FDodgeStats& Fallback, int32 PrevBucket, FVector2D& OutOffset, float& OutConfidence) const
	{
		const bool bHasPrev = IsValidBucket(PrevBucket);

		float Weights[BucketCount] = {};
		auto AccumulateWeights = [&Weights, bHasPrev, PrevBucket](const FDodgeStats& Stats, float Scale)
			{
				for (int32 i = 0; i < BucketCount; ++i)
				{
					if (!bHasPrev)
					{
						// Use the overall bucket weights when the previous dodge bucket is not recorded
						if (Stats.Buckets.IsValidIndex(i)) Weights[i] += Stats.Buckets[i].Weight * Scale;
					}
					else if (Stats.Transitions.Num() == BucketCount * BucketCount)
					{
						// Use the transition row of the previous dodge when available
						Weights[i] += Stats.Transitions[PrevBucket * BucketCount + i] * Scale;
					}
				}
			};
		AccumulateWeights(*this, 1.f);
		AccumulateWeights(Fallback, FallbackScale);

		float Total = 0.f;
		int32 Best = 0;
		for (int32 i = 0; i < BucketCount; ++i)
		{
			Total += Weights[i];
			if (Weights[i] > Weights[Best]) Best = i;
		}

		const float RequiredShare = bHasPrev ? MinShare : OpenerMinShare;
		if (Weights[Best] < MinWeight || Weights[Best] < Total * RequiredShare) return false;
		if (static_cast<EDodgeBucket>(Best) == EDodgeBucket::Stay) return false;

		// Dodge distance: mean offset of the predicted direction over both stats
		float OffsetWeight = 0.f;
		FVector2D OffsetSum = FVector2D::ZeroVector;
		auto AccumulateOffset = [&OffsetWeight, &OffsetSum, Best](const FDodgeStats& Stats, float Scale)
			{
				if (!Stats.Buckets.IsValidIndex(Best)) return;
				const FDodgeBucketStat& Stat = Stats.Buckets[Best];
				OffsetWeight += Stat.Weight * Scale;
				OffsetSum += Stat.MeanOffset * Stat.Weight * Scale;
			};
		AccumulateOffset(*this, 1.f);
		AccumulateOffset(Fallback, FallbackScale);

		if (OffsetWeight <= KINDA_SMALL_NUMBER) return false;

		OutOffset = OffsetSum / OffsetWeight;
		// Share of the predicted direction, discounted while the evidence is small
		OutConfidence = Weights[Best] / (Total + ConfidenceK);
		return true;
	}
};

// Snapshot of an attack whose outcome is learned later
struct FAttackSample
{
	bool bActive = false;
	int32 PatternIdx = INDEX_NONE;
	int32 AttackIdx = INDEX_NONE;
 
	// Attack axis: horizontal direction from the attacker to the target at commit time
	FVector Forward = FVector::ForwardVector;
	// Target location the aim was based on (before the learned correction)
	FVector Origin = FVector::ZeroVector;
	
	// Previous dodge used for the prediction at commit time; the outcome is learned against the same context
	int32 PrevBucket = INDEX_NONE;
	// Learned correction decided once at commit time (melee re-aims reuse it)
	FVector AimShift = FVector::ZeroVector;
 
	// Filled in when the sample is resolved
	FVector ResolvedLocation = FVector::ZeroVector;
 
	void Begin(int32 NewPatternIdx, int32 NewAttackIdx, const FVector& NewForward, const FVector& NewOrigin, int32 NewPrevBucket)
	{
		*this = FAttackSample();
		bActive = !NewForward.IsNearlyZero();
		PatternIdx = NewPatternIdx;
		AttackIdx = NewAttackIdx;
		Forward = NewForward;
		Origin = NewOrigin;
		PrevBucket = NewPrevBucket;
	}
};

// Player action record for AI decision making
USTRUCT(BlueprintType)
struct FPlayerActionRecord
{
    GENERATED_BODY()
	
	// Dodge habit across all swings; supports swings that have few samples
	UPROPERTY(VisibleAnywhere)
	FDodgeStats DodgeFallback;

	// Dodge bucket of the previous lunge in the pattern currently being executed (INDEX_NONE at pattern start)
	int32 PatternPrevBucket = INDEX_NONE;

	// Lunge attack currently being tracked
	FAttackSample PendingSample;
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
};