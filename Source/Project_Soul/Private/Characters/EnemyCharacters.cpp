// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/EnemyCharacters.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/Combat/EnemyCombatComponent.h"
#include "Engine/AssetManager.h"
#include "DataAssets/StartUpData/DataAsset_EnemyStartUpData.h"
#include "Components/UI/EnemyUIComponent.h"
#include "Components/WidgetComponent.h"
#include "Widgets/KwangWidgetBase.h"

#include "KwangDebugHelper.h"
AEnemyCharacters::AEnemyCharacters()
{
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 2. 컨트롤러 회전 사용 여부 (Pitch, Roll, Yaw)
	// 컨트롤러(AI)가 회전한다고 해서 캐릭터 몸체(Mesh)가 즉각적으로 같이 회전하지 않도록 모두 false
	// 이게 true면 캐릭터가 컨트롤러 방향으로 즉시 돌아가버림
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	// 3. 캐릭터 무브먼트 컴포넌트 설정
	// 컨트롤러가 원하는 회전 방향으로 몸을 돌릴지 여부
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	// 4. 이동 방향으로 캐릭터 회전
	// 캐릭터가 움직이는 방향(Velocity)을 향해 몸이 자동으로 돌아가게 함
	// 예를 들어, 왼쪽으로 이동하면 캐릭터 몸도 왼쪽을 쳐다보며 걷게 된다.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 5. 회전 속도 설정
	// 캐릭터가 몸을 돌릴 때의 속도 (Pitch, Yaw, Roll) 순서이며,
	// 현재 Yaw(좌우 회전)가 180도로 되어 있어 초당 180도 속도로 부드럽게 회전
	GetCharacterMovement()->RotationRate = FRotator(0.f, 180.f, 0.f);

	// 6. 이동 속도 및 마찰력 설정
	// 적의 최대 걷기 속도를 300(약간 느린 걸음)으로 제한
	GetCharacterMovement()->MaxWalkSpeed = 300.f;

	// 멈출 때의 감속도
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f;

	EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>("EnemyCombatComponent");

	EnemyUIComponent = CreateDefaultSubobject<UEnemyUIComponent>("EnemyUIComponent");

	EnemyHealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("EnemyHealthWidgetComponent");
	EnemyHealthWidgetComponent->SetupAttachment(GetMesh());
}

UPawnCombatComponent* AEnemyCharacters::GetPawnCombatComponent() const
{
	return EnemyCombatComponent;
}

UPawnUIComponent* AEnemyCharacters::GetPawnUIComponent() const
{
	return EnemyUIComponent;
}

UEnemyUIComponent* AEnemyCharacters::GetEnemyUIComponent() const
{
	return EnemyUIComponent;
}

void AEnemyCharacters::BeginPlay()
{
	Super::BeginPlay();

	if (UKwangWidgetBase* HealthWidget = Cast<UKwangWidgetBase>(EnemyHealthWidgetComponent->GetUserWidgetObject()))
	{
		HealthWidget->InitEnemyCreatedWidget(this);
	}
}

void AEnemyCharacters::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitEnemyStartUpData();
}

void AEnemyCharacters::InitEnemyStartUpData()
{
	if (CharacterStartUpData.IsNull())
	{
		return;
	}

	UAssetManager::GetStreamableManager().RequestAsyncLoad(
		CharacterStartUpData.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda(
			[this]()
			{
				if (UDataAsset_StartUpDataBase* LoadedData = CharacterStartUpData.Get())
				{
					LoadedData->GiveToAbilitySystemComponent(KwangAbilitySystemComponent);
				}
			}
		)
	);
}
