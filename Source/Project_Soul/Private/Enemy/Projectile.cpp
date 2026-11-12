#include "Enemy/Projectile.h"
#include "Enemy/Enemy.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set up the collision component as the root component
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetBoxExtent(FVector(55.f, 5.f, 5.f));
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RootComponent = CollisionComponent;

	// Set up the mesh component for visual representation
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 
	// Speed is injected by the shooter in Launch(); homing is opt-in through InitHoming()
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->bIsHomingProjectile = false;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	SetLifeSpan(LifeSpan);
	
	// Ignore the instigator and bind the hit event
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnOverlapBegin);
	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
}

void AProjectile::Launch(const FVector& Direction, float NewSpeed)
{
	// MaxSpeed equal to the launch speed keeps homing from accelerating the projectile,
	// so the flight time stays what the shooter predicted
	ProjectileMovement->InitialSpeed = NewSpeed;
	ProjectileMovement->MaxSpeed = NewSpeed;
	ProjectileMovement->Velocity = Direction.GetSafeNormal() * NewSpeed;
	ProjectileMovement->Activate();
}

void AProjectile::InitHoming(AActor* Target, const FVector& PredictedOffset)
{
	if (!Target) return;

	HomingActor = Target;
	FireTargetLocation = Target->GetActorLocation();
	PredictedDir = PredictedOffset.GetSafeNormal2D();
	PredictedDistance = PredictedOffset.Size2D();

	if (!HomingTarget)
	{
		HomingTarget = NewObject<USceneComponent>(this, TEXT("HomingTarget"));
		HomingTarget->RegisterComponent();
	}
	HomingTarget->SetWorldLocation(FireTargetLocation + PredictedOffset);

	ProjectileMovement->HomingTargetComponent = HomingTarget;
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	ProjectileMovement->bIsHomingProjectile = true;
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ProjectileMovement->bIsHomingProjectile) return;

	AActor* Target = HomingActor.Get();
	if (!Target)
	{
		ProjectileMovement->bIsHomingProjectile = false;
		return;
	}

	// Only the part of the fire-time prediction the target has not covered yet is added,
	// so a target that dodges as predicted is not corrected twice
	const FVector TargetLocation = Target->GetActorLocation();
	const float Covered = FVector::DotProduct(TargetLocation - FireTargetLocation, PredictedDir);
	const float Remaining = FMath::Clamp(PredictedDistance - Covered, 0.f, PredictedDistance);
	const FVector AimPoint = TargetLocation + PredictedDir * Remaining;

	const FVector ToTarget = AimPoint - GetActorLocation();
	const FVector CurrentDirection = ProjectileMovement->Velocity.GetSafeNormal();

	// Disable projectile homing when the projectile has passed the target
	if (FVector::DotProduct(CurrentDirection, ToTarget.GetSafeNormal()) < 0.1f)
	{
		ProjectileMovement->bIsHomingProjectile = false;
		return;
	}

	const float Distance = ToTarget.Size();
	FVector HomingPoint = AimPoint;

	// Lead with the target's velocity only once the fire-time prediction is used up
	if (Remaining <= KINDA_SMALL_NUMBER)
	{
		FVector TargetVelocity = Target->GetVelocity();
		TargetVelocity.Z = 0.f;
		const float CurrentSpeed = FMath::Max(ProjectileMovement->Velocity.Size(), 1.f);
		HomingPoint += TargetVelocity * (Distance / CurrentSpeed);
	}

	HomingTarget->SetWorldLocation(HomingPoint);

	// Gradually reduce homing acceleration as the projectile gets closer to the target to create a smoother trajectory
	if (Distance < 500.f)
	{
		const float Alpha = FMath::Clamp(Distance / 500.f, 0.3f, 1.f);
		ProjectileMovement->HomingAccelerationMagnitude = FMath::Lerp(0.f, HomingAcceleration, Alpha);
	}
}

// Handle projectile overlap with other actors, applying damage and destroying the projectile
void AProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Ignore other enemies
	if (Cast<AEnemy>(OtherActor)) return;

	// The shooter may already be gone when the projectile lands
	AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
	UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorController, this, nullptr);

	Destroy();
}

// Destroy the projectile when it is blocked by world geometry
void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}