#include "Enemy/NormalEnemy_Ranged.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

ANormalEnemy_Ranged::ANormalEnemy_Ranged()
{
	
}

void ANormalEnemy_Ranged::FireProjectile_Implementation()
{
	
}

// Start the decision timer with a dynamic interval based on distance to the target
// Allowing the ranged enemy more time to get away from the target before deciding next action
void ANormalEnemy_Ranged::StartDecisionTimer()
{
	float DecisionTime = 0.5f;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (AIC && AIC->GetFocusActor()) 
	{
		float DistanceToTarget = FVector::Dist(GetActorLocation(), AIC->GetFocusActor()->GetActorLocation());
		DecisionTime = FMath::GetMappedRangeValueClamped(FVector2D(0.f, MinCombatRange), FVector2D(3.f, 0.5f), DistanceToTarget);
	}

	GetWorldTimerManager().SetTimer(DecisionTimerHandle, this, &ANormalEnemy_Ranged::DecideNextAction, DecisionTime, true);
}

// Adjust the movement speed based on distance to the target, encouraging the ranged enemy to maintain optimal combat range
void ANormalEnemy_Ranged::SetSpeedByDistance(float Distance)
{
	Super::SetSpeedByDistance(Distance);

	// If the distance is less than 0.7 times the minimum combat range, set the speed to RunSpeed
	if (Distance < 0.7 * MinCombatRange)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
	// If the distance is between 0.7 and 1.0 times the minimum combat range, interpolate the speed between RunSpeed and WalkSpeed
	else if (Distance < MinCombatRange)
	{
		GetCharacterMovement()->MaxWalkSpeed = FMath::GetMappedRangeValueClamped(
			FVector2D(0.7f * MinCombatRange, MinCombatRange), FVector2D(RunSpeed, WalkSpeed), Distance);
	}
}