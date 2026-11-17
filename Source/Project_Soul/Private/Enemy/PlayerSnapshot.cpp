#include "Enemy/PlayerSnapshot.h"
#include "Characters/KwangHeroCharacters.h"
#include "GameFramework/Actor.h"

namespace
{
	constexpr float MoveSpeedThreshold = 10.f;
}

FPlayerSnapshot FPlayerSnapshot::Capture(const AActor* Player)
{
	FPlayerSnapshot Snapshot;
	if (!Player) return Snapshot;

	Snapshot.bValid = true;
	Snapshot.Location = Player->GetActorLocation();
	Snapshot.Velocity = Player->GetVelocity();

	// Only place that reads the player class.
	const AKwangHeroCharacters* Hero = Cast<const AKwangHeroCharacters>(Player);

	// Guard and attack come first: an attack animation can move faster than the roll threshold
	const float Speed = Snapshot.Velocity.Size2D();
	if (Hero)
	{
		Snapshot.bInvincible = Hero->IsInvincible();

		if (Hero->IsGuarding())
		{
			Snapshot.Action = EPlayerAction::Guarding;
			return Snapshot;
		}
		if (Hero->IsAttacking())
		{
			Snapshot.Action = EPlayerAction::Attacking;
			return Snapshot;
		}
		if (Hero->IsRolling())
		{
			Snapshot.Action = EPlayerAction::Rolling;
			return Snapshot;
		}
	}

	if (Snapshot.Velocity.Size2D() >= MoveSpeedThreshold)
	{
		Snapshot.Action = EPlayerAction::Moving;
	}

	return Snapshot;
}
