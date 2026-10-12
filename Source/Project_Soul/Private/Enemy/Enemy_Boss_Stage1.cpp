#include "Enemy/Enemy_Boss_Stage1.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/BoxComponent.h"
#include "Delegates/Delegate.h"

// Sets default values
AEnemy_Boss_Stage1::AEnemy_Boss_Stage1()
{
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	RightWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWeaponMesh"));
	LeftWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWeaponMesh"));
}

// Called after the actor's components have been initialized
void AEnemy_Boss_Stage1::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Attach the weapon mesh to the character's hand socket
	RightWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightWeaponSocketName);
	LeftWeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, LeftWeaponSocketName);

	// Bind the weapon hitbox overlap event and disable it initially to prevent unintended collisions
	RightWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &AEnemy_Boss_Stage1::OnWeaponHitboxOverlap);
	LeftWeaponHitbox->OnComponentBeginOverlap.AddDynamic(this, &AEnemy_Boss_Stage1::OnWeaponHitboxOverlap);
}

// Called when the game starts or when spawned
void AEnemy_Boss_Stage1::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the behavior tree and update combat range values in the blackboard
	AAIController* AIC = Cast<AAIController>(GetController());
	AIC->RunBehaviorTree(BTAsset);
	AIC->GetBlackboardComponent()->SetValueAsFloat(FName("MinCombatRange"), MinCombatRange);
	AIC->GetBlackboardComponent()->SetValueAsFloat(FName("MaxCombatRange"), MaxCombatRange);
	
	// Start a timer to continuously update the boss's movement speed based on the player's distance
	GetWorldTimerManager().SetTimer(UpdateDistanceTimerHandle, this, &AEnemy_Boss_Stage1::UpdateDistance, 0.1f, true);
}

void AEnemy_Boss_Stage1::StartBossFight()
{
	ResumeCombat();

	if (!BossHealthBarClass || BossHealthBarWidget) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	BossHealthBarWidget = CreateWidget<UBoss_HealthBarWidget>(PC, BossHealthBarClass);

	if (!BossHealthBarWidget) return;

	BossHealthBarWidget->SetBossName(BossDisplayName);
	BossHealthBarWidget->SetHealthPercent(MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f);
	BossHealthBarWidget->AddToViewport();
}

void AEnemy_Boss_Stage1::EndBossFight()
{
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->RemoveFromParent();
		BossHealthBarWidget = nullptr;
	}
}

void AEnemy_Boss_Stage1::NotifyHealthBarUpdate(float NewCurrentHealth, float NewMaxHealth)
{
	if (!BossHealthBarWidget) return;

	BossHealthBarWidget->SetHealthPercent(NewMaxHealth > 0.f ? NewCurrentHealth / NewMaxHealth : 0.f);

	if (NewCurrentHealth <= 0.f)
	{
		EndBossFight();
	}
}

// Update the boss's movement speed based on the distance to the player and update the blackboard
void AEnemy_Boss_Stage1::UpdateDistance()
{
	if (CharacterState != EEnemyState::InCombat) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	SetSpeedByDistance(Distance);
	Cast<AAIController>(GetController())->GetBlackboardComponent()->SetValueAsFloat(FName("DistanceToTarget"), Distance);
}

// Base decision logic for enemy's next action
void AEnemy_Boss_Stage1::DecideNextAction()
{
	if (CharacterState != EEnemyState::InCombat) return;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	AActor* Target = Cast<AActor>(AIC->GetFocusActor());
	if (!Target) return;

	float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

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

// Handle resuming to combat state, refocusing on the player and updates the blackboard
void AEnemy_Boss_Stage1::ResumeCombat()
{
	Super::ResumeCombat();

	AAIController* AIC = Cast<AAIController>(GetController());
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!AIC || !Player) return;

	AIC->SetFocus(Player);

	if (UBlackboardComponent* BBComp = AIC->GetBlackboardComponent())
	{
		BBComp->SetValueAsObject(FName("TargetActor"), Player);
	}
}