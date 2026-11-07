#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
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
	float Speed = 600.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float HomingAcceleration = 1000.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float LifeSpan = 10.f;

	virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void OnHit(UPrimitiveComponent* HitCombp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, FVector NormalImpulse, const FHitResult& Hit);

public:
	void SetDamage(float DamageAmount) { Damage = DamageAmount; }
};
