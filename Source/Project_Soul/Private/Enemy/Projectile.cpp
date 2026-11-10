#include "Enemy/Projectile.h"
#include "Enemy/Enemy.h"
#include "Components/BoxComponent.h"
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

	// Set up the projectile movement component for handling movement and homing behavior
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = Speed;
    ProjectileMovement->MaxSpeed = 1000.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bAutoActivate = false;
    ProjectileMovement->bIsHomingProjectile = true;
    ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
    SetLifeSpan(LifeSpan);

    // Ignore the instigator and bind the hit event
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnOverlapBegin);

	// Create a homing target component and set it to the player's location
    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (Player)
    {
		HomingTarget = NewObject<USceneComponent>(this, TEXT("HomingTarget"));
		HomingTarget->RegisterComponent();
		HomingTarget->SetWorldLocation(Player->GetActorLocation());
        ProjectileMovement->HomingTargetComponent = HomingTarget;
    }
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ProjectileMovement->bIsHomingProjectile) return;
    if (!ProjectileMovement->HomingTargetComponent.IsValid()) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    FVector ToTarget = Player->GetActorLocation() - GetActorLocation();
    FVector CurrentDirection = ProjectileMovement->Velocity.GetSafeNormal();

    // Disable projectile homing when the projectile has missed the target
    if (FVector::DotProduct(CurrentDirection, ToTarget.GetSafeNormal()) < 0.1f)
    {
        ProjectileMovement->bIsHomingProjectile = false;
        
        DrawDebugSphere(GetWorld(), Player->GetActorLocation(), 20.f, 12, FColor::Red, false, 3.f);

        // Record player dodge location
        AEnemy* Enemy = Cast<AEnemy>(GetInstigator());
        if (Enemy)
        {
            FVector ToPlayer = (Player->GetActorLocation() - Enemy->GetPlayerActionRecord().GetAttackDirection());
            FVector ToPlayerDir = GetActorTransform().InverseTransformVectorNoScale(ToPlayer);

            Enemy->GetPlayerActionRecord().RecordDodge(ToPlayerDir);
        }
        return;
    }

	// Calculate lead for moving targets to improve homing accuracy
	FVector TargetVelocity = Player->GetVelocity();
	float Distance = ToTarget.Size();
	float TimeToTarget = Distance / ProjectileMovement->InitialSpeed;

	HomingTarget->SetWorldLocation(Player->GetActorLocation() + TargetVelocity * TimeToTarget);

	// Gradually reduce homing acceleration as the projectile gets closer to the target to create a smoother trajectory
    if (Distance < 500.f)
    {
        float Alpha = FMath::Clamp(Distance / 500.f, 0.3f, 1.f);

        ProjectileMovement->HomingAccelerationMagnitude = FMath::Lerp(0.f, HomingAcceleration, Alpha);
    }
}

// Handle projectile overlap with other actors, applying damage and destroying the projectile
void AProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Ignore other enemies
    if (Cast<AEnemy>(OtherActor)) 
    {
        return;
    }

    UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigator()->GetController(), this, nullptr);
   
    UE_LOG(LogTemp, Warning, TEXT("Hit: %s / Damage: %.1f"), *OtherActor->GetName(), Damage);

    Destroy();
}

// Handle hit events, destroying the projectile upon impact
void AProjectile::OnHit(UPrimitiveComponent* HitCombp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}