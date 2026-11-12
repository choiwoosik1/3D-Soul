#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class PROJECT_SOUL_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	// Projectile components
    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UBoxComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    class USceneComponent* HomingTarget;

	// Projectile properties
    UPROPERTY(EditAnywhere, Category = "Combat")
    float Damage = 10.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float HomingAcceleration = 1400.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float LifeSpan = 4.f;

	virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);

public:
	void SetDamage(float DamageAmount) { Damage = DamageAmount; }

	// Fire the projectile at a constant speed so the real flight time matches the shooter's prediction
	void Launch(const FVector& Direction, float NewSpeed);

	// Home in on the target while keeping the shooter's learned aim shift applied
	void InitHoming(AActor* Target, const FVector& NewAimShift);

private:
	TWeakObjectPtr<AActor> HomingActor;

    // Fire-time prediction, consumed as the target actually moves along it
    FVector FireTargetLocation = FVector::ZeroVector;
    FVector PredictedDir = FVector::ZeroVector;
    float PredictedDistance = 0.f;
};