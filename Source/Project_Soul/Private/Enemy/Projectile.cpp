#include "Enemy/Projectile.h"
#include "Enemy/Enemy.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
    CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetBoxExtent(FVector(15.f, 15.f, 15.f));
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RootComponent = CollisionComponent;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
    CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (Player)
    {
		HomingTarget = NewObject<USceneComponent>(this, TEXT("HomingTarget"));
		HomingTarget->RegisterComponent();
		HomingTarget->SetWorldLocation(Player->GetActorLocation());
        ProjectileMovement->HomingTargetComponent = HomingTarget;
    }
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ProjectileMovement->bIsHomingProjectile) return;
    if (!ProjectileMovement->HomingTargetComponent.IsValid()) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    FVector ToTarget = Player->GetActorLocation() - GetActorLocation();
    FVector CurrentDirection = ProjectileMovement->Velocity.GetSafeNormal();

    if (FVector::DotProduct(CurrentDirection, ToTarget.GetSafeNormal()) < 0.f)
    {
        ProjectileMovement->bIsHomingProjectile = false;

        AEnemy* Enemy = Cast<AEnemy>(GetInstigator());
        if (Enemy)
        {
            FVector ToPlayer = (Player->GetActorLocation() - Enemy->GetPlayerActionRecord().GetAttackDirection());
            FVector ToPlayerDir = GetActorTransform().InverseTransformVector(ToPlayer);

            Enemy->GetPlayerActionRecord().RecordDodge(ToPlayerDir);
        }
        return;
    }

	FVector TargetVelocity = Player->GetVelocity();
	float Distance = ToTarget.Size();
	float TimeToTarget = Distance / ProjectileMovement->InitialSpeed;

	HomingTarget->SetWorldLocation(Player->GetActorLocation() + TargetVelocity * TimeToTarget);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == GetInstigator() || Cast<AEnemy>(OtherActor)) 
    {
        Destroy();
        return;
    }

    UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigator()->GetController(), this, nullptr);
   
    UE_LOG(LogTemp, Warning, TEXT("Hit: %s / Damage: %.1f"), *OtherActor->GetName(), Damage);

    Destroy();
}