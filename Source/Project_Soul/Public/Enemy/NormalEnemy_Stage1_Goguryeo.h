#pragma once

#include "CoreMinimal.h"
#include "Enemy/NormalEnemy_Ranged.h"
#include "NormalEnemy_Stage1_Goguryeo.generated.h"

UCLASS()
class PROJECT_SOUL_API ANormalEnemy_Stage1_Goguryeo : public ANormalEnemy_Ranged
{
	GENERATED_BODY()
	
public:
	ANormalEnemy_Stage1_Goguryeo();

protected:
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileSpeed = 1000.f;
	
	// Upper bound on the predicted flight time used for lead
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float MaxLeadTime = 2.f;

	// Target speed above this is clamped so a roll or a fall is not extrapolated for lead
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float MaxLeadSpeed = 600.f;

	// Target faster than this at fire time is treated as rolling (between walk 400 and roll 500+)
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float DodgeSpeedThreshold = 500.f;

	// Fixed aim distance along the roll direction when the target is rolling at fire time
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float RollLeadDistance = 280.f;

	// Fastest lateral speed a dodge can reach; errors beyond this speed x flight time are not learned
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "0.0"))
	float MaxDodgeSpeed = 800.f;

	// Dodge bucket of the last learned shot; the next shot is predicted from it (kept across respawns)
	int32 LastShotBucket = INDEX_NONE;


	virtual void PostInitializeComponents() override;

	virtual void DecideNextAction() override;

	virtual void FireProjectile_Implementation() override;
};