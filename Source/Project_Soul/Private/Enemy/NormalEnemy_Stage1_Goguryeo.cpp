#include "Enemy/NormalEnemy_Stage1_Goguryeo.h"
#include "Enemy/Projectile.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"

namespace
{
	// Smallest positive t solving |ToTarget + TargetVelocity * t| = ProjectileSpeed * t.
	// Falls back to straight-line flight time when no intercept exists.
	float ComputeInterceptTime(const FVector& ToTarget, const FVector& TargetVelocity, float ProjectileSpeed)
	{
		const float A = TargetVelocity.SizeSquared() - ProjectileSpeed * ProjectileSpeed;
		const float B = 2.f * FVector::DotProduct(ToTarget, TargetVelocity);
		const float C = ToTarget.SizeSquared();
		const float Fallback = ToTarget.Size() / ProjectileSpeed;

		if (FMath::IsNearlyZero(A))
		{
			return B < 0.f ? -C / B : Fallback;
		}

		const float Discriminant = B * B - 4.f * A * C;
		if (Discriminant < 0.f) return Fallback;

		const float Root = FMath::Sqrt(Discriminant);
		const float T1 = (-B - Root) / (2.f * A);
		const float T2 = (-B + Root) / (2.f * A);

		if (T1 > 0.f && T2 > 0.f) return FMath::Min(T1, T2);
		if (T1 > 0.f) return T1;
		if (T2 > 0.f) return T2;
		return Fallback;
	}
}

// Sets default values
ANormalEnemy_Stage1_Goguryeo::ANormalEnemy_Stage1_Goguryeo()
{
	LeftWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftWeaponMesh"));
}

// Called after the actor's components have been initialized
void ANormalEnemy_Stage1_Goguryeo::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Attach the weapon mesh to the character's hand socket
	LeftWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, LeftWeaponSocketName);
	LeftWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &ANormalEnemy_Stage1_Goguryeo::OnWeaponHitboxOverlap);
}

// Base decision logic for enemy's next action
void ANormalEnemy_Stage1_Goguryeo::DecideNextAction()
{ 
	if (CharacterState != EEnemyState::InCombat) return;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	AActor* Target = Cast<AActor>(AIC->GetFocusActor());
	if (!Target) return;

	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	// If the target is within the melee attack pattern's range, perform that attack immediately
	if (Distance <= Patterns[1].AttackRange)
	{
		PerformAttackPattern(1);
		return;
	}

	// Find valid attack patterns based on distance to the target
	TArray<int32> ValidAttackPatterns;
	for (int32 i = 0; i < Patterns.Num(); i++)
	{
		if (Distance <= Patterns[i].AttackRange)
			ValidAttackPatterns.Add(i);
	}

	// If there are valid attack patterns, randomly select one and perform the attack
	if (ValidAttackPatterns.Num() > 0)
	{
		int32 SelectedIdx = ValidAttackPatterns[FMath::RandRange(0, ValidAttackPatterns.Num() - 1)];
		PerformAttackPattern(SelectedIdx);
		return;
	}
}

// Fire a projectile at the predicted intercept point, shifted sideways by the learned dodge habit,
// and schedule a dodge sample at the moment the projectile is expected to arrive
void ANormalEnemy_Stage1_Goguryeo::FireProjectile_Implementation()
{
	if (!ProjectileClass) return;
	if (!Patterns.IsValidIndex(PatternIdx) || !Patterns[PatternIdx].Attacks.IsValidIndex(AttackIdx)) return;

	// Capture the swing before AttackIdx advances; the delayed sample must be learned for this swing
	const int32 FiredPatternIdx = PatternIdx;
	const int32 FiredAttackIdx = AttackIdx++;
	const FAttackProperties& Attack = Patterns[FiredPatternIdx].Attacks[FiredAttackIdx];

	const float Speed = FMath::Max(ProjectileSpeed, 1.f);
	const FVector MuzzleLocation = LeftWeaponMesh->GetSocketLocation(MuzzleSocketName);
	FRotator FireRotation = LeftWeaponMesh->GetSocketRotation(MuzzleSocketName);
	FVector PredictedOffset = FVector::ZeroVector;

	AAIController* AIC = Cast<AAIController>(GetController());
	AActor* Target = AIC ? AIC->GetFocusActor() : nullptr;
	if (Target)
	{
		const FVector TargetLocation = Target->GetActorLocation();
		const FVector RawVelocity = Target->GetVelocity();

		// Roll speed lasts only for the roll, so extrapolating it over the flight overshoots;
		// a target rolling at fire time is aimed a fixed distance along the roll direction instead
		const bool bTargetRolling = RawVelocity.Size2D() > DodgeSpeedThreshold;
		float LeadTime = 0.f;
		FVector Intercept;

		if (bTargetRolling)
		{
			Intercept = TargetLocation + RawVelocity.GetSafeNormal2D() * RollLeadDistance;
		}
		else
		{
			// Horizontal velocity only, clamped so falls are not extrapolated
			const FVector LeadVelocity = FVector(RawVelocity.X, RawVelocity.Y, 0.f).GetClampedToMaxSize(MaxLeadSpeed);
			LeadTime = FMath::Min(ComputeInterceptTime(TargetLocation - MuzzleLocation, LeadVelocity, Speed), MaxLeadTime);
			Intercept = TargetLocation + LeadVelocity * LeadTime;
		}

		// Attack axis for dodge learning: horizontal direction from the muzzle to the predicted point
		const FVector Forward = (Intercept - MuzzleLocation).GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);

		// Consecutive shots form the sequence: predict from how the target dodged the last learned shot.
		// The fixed roll distance already covers the dodge, so the learned shift is not added on top of it.
		const int32 PrevBucket = LastShotBucket;
		const FVector AimLocation = bTargetRolling ? Intercept : ApplyAimOffset(Intercept, Forward, Attack, PrevBucket);
		const float ShiftRight = FVector::DotProduct(AimLocation - Intercept, Right);
		PredictedOffset = AimLocation - TargetLocation;

		FireRotation = (AimLocation - MuzzleLocation).Rotation();

		// Debug: blue = target now, yellow = predicted point, green = corrected aim (remove once tuned)
		DrawDebugSphere(GetWorld(), TargetLocation, 20.f, 12, FColor::Blue, false, 3.f);
		DrawDebugSphere(GetWorld(), Intercept, 20.f, 12, FColor::Yellow, false, 3.f);
		DrawDebugSphere(GetWorld(), AimLocation, 20.f, 12, FColor::Green, false, 3.f);

		// Sample where the target actually is when the projectile is expected to arrive,
		// measured from the uncorrected predicted point so the correction never feeds back into the data
		FAttackSample Sample;
		Sample.Begin(FiredPatternIdx, FiredAttackIdx, Forward, Intercept, PrevBucket);

		const float FlightTime = FMath::Max(FVector::Dist(MuzzleLocation, AimLocation) / Speed, 0.05f);
		const float Distance = FVector::Dist(MuzzleLocation, TargetLocation);
		const float RawSpeed = RawVelocity.Size2D();
		const TWeakObjectPtr<AActor> WeakTarget = Target;

		FTimerHandle SampleHandle;
		GetWorldTimerManager().SetTimer(SampleHandle, FTimerDelegate::CreateWeakLambda(this,
			[this, Sample, Right, WeakTarget, Distance, RawSpeed, LeadTime, ShiftRight, FlightTime, bTargetRolling]() mutable
			{
				const AActor* SampledTarget = WeakTarget.Get();
				if (!SampledTarget) return;

				const FVector Actual = SampledTarget->GetActorLocation();
				const FVector Error = Actual - Sample.Origin;
				const float ErrorForward = FVector::DotProduct(Error, Sample.Forward);
				const float ErrorRight = FVector::DotProduct(Error, Right);

				// Debug: red = target when the projectile should arrive; red vs yellow is the prediction error
				DrawDebugSphere(GetWorld(), Actual, 20.f, 12, FColor::Red, false, 3.f);

				// MissR = lateral miss of the aim point (with homing on, not the actual miss)
				UE_LOG(LogTemp, Warning, TEXT("ShotSample\tRoll=%d\tDist=%.0f\tSpeed=%.0f\tLeadT=%.2f\tShiftR=%.0f\tErrF=%.0f\tErrR=%.0f\tMissR=%.0f"),
					bTargetRolling, Distance, RawSpeed, LeadTime, ShiftRight, ErrorForward, ErrorRight, ErrorRight - ShiftRight);

				// Rolling shots are aimed by the fixed roll distance and measure a different thing; keep them out of the stats
				if (bTargetRolling) return;

				// Discard samples no dodge could produce within this flight time (teleports, respawns)
				if (FMath::Abs(ErrorRight) > MaxDodgeSpeed * FlightTime) return;

				// A straight shot can only be corrected sideways, so learn the lateral component only
				Sample.ResolvedLocation = Sample.Origin + Right * ErrorRight;
				LastShotBucket = RecordSample(Sample);
			}), FlightTime, false);
	}

	// Spawn the projectile with the calculated rotation
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, MuzzleLocation, FireRotation, SpawnParams);
	if (!Projectile) return;

	Projectile->SetDamage(BaseDamage * Attack.DamageMultiplier);

	Projectile->InitHoming(Target, PredictedOffset);

	Projectile->Launch(FireRotation.Vector(), Speed);
}