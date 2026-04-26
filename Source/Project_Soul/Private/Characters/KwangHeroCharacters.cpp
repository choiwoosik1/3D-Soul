// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/KwangHeroCharacters.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "Components/Input/KwangInputComponent.h"
#include "KwangGameplayTags.h"
#include "AbilitySystem/KwangAbilitySystemComponent.h" // GAS의 '뇌' 역할을 하는 컴포넌트
#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "KwangDebugHelper.h"

// Sets default values
AKwangHeroCharacters::AKwangHeroCharacters()
{
	// 1. 캐릭터의 물리 충돌 영역(캡슐) 크기 세팅 (반지름 42, 절반 높이 96)
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// 2. 캐릭터의 회전이 카메라(컨트롤러)의 회전을 따라갈 것인가 ? -> "안 따라간다(false)"
	// 이게 true면 마우스를 돌릴 때마다 캐릭터 몸통도 그 방향으로 뻣뻣하게 같이 돈다.
	// RPG 게임은 마우스를 돌리면 캐릭터 몸은 가만히 있고 카메라만 주위를 맴돌아야 하므로 전부 false로 끔.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	 
	// 3. 스프링암 컴포넌트를 메모리에 생성하고 이름을 "CameraBoom"으로 지정
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	// 4. 생성한 스프링암을 캐릭터의 최상위 부모(Root)인 캡슐 컴포넌트에 갖다 붙임
	CameraBoom->SetupAttachment(GetRootComponent());

	// 5. 스프링암(셀카봉)의 길이를 200 언리얼 유닛(cm)으로 설정
	CameraBoom->TargetArmLength = 200.f;

	// 6. 카메라가 캐릭터의 정중앙 뒤통수를 가리지 않도록, 셀카봉의 끝부분을 살짝 우측 상단으로 옮김.
	// FVector(X:앞뒤, Y:좌우, Z:위아래). 즉, 우측으로 55, 위로 65만큼 이동시킨 '오버 숄더(어깨너머)' 뷰 세팅
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);

	// 7.마우스를 움직일 때(컨트롤러 회전), 이 셀카봉(스프링암)이 그 마우스 회전값을 받아서 같이 돌아갈 것인가? -> "돌아간다(true)"
	CameraBoom->bUsePawnControlRotation = true;

	// 8. 실제 화면을 비춰줄 카메라 부품을 메모리에 생성하고 이름을 "FollowCamera"로 지정
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));

	// 9. 이 카메라를 아까 만든 셀카봉(CameraBoom)에 갖다 붙임
	// 그냥 붙이는 게 아니라, USpringArmComponent::SocketName 이라는 '셀카봉의 맨 끝부분(소켓)'에 정확히 달아줌
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// 10. 카메라는 마우스 회전값을 직접 받을 것인가? -> "안 받는다(false)"
	// 카메라는 이미 마우스에 따라 도는 셀카봉 끝에 매달려 있으므로, 굳이 자신까지 이중으로 회전할 필요가 없음
	FollowCamera->bUsePawnControlRotation = false;

	// GetCharacterMovement(): 언리얼 엔진이 캐릭터에게 기본으로 달아준 '이동 관리 컴포넌트'를 불러옴
	// 1. 캐릭터가 이동하려는 방향으로 몸(Mesh)을 자동으로 회전시킬 것인가? -> "그렇다(true)"
	// 아까 마우스 회전을 꺼뒀기 때문에, 이제 캐릭터는 마우스가 아니라 자기가 걸어가는 방향(W,A,S,D)을 쳐다보며 걷게 됨.
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	// FRotator(Pitch, Yaw, Roll) 중 Yaw(Z축, 좌우 돌림) 값을 500으로 설정. (1초에 500도 회전)
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);

	// 3. 캐릭터의 최대 걷기(이동) 속도를 400 언리얼 유닛(cm/s)으로 제한
	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	// 4. 키보드(이동 키)에서 손을 뗐을 때 적용되는 브레이크(감속)의 힘
	// 수치가 높을수록 얼음판처럼 미끄러지지 않고, 바닥에 발을 딱 붙이고 즉시 멈춰 섬.
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	HeroCombatComponent = CreateDefaultSubobject<UHeroCombatComponent>(TEXT("HeroCombatComponent"));
}

void AKwangHeroCharacters::PossessedBy(AController* NewController)
{
	// 1. 부모 클래스의 기본 빙의 로직을 먼저 실행해서 뼈대 세팅
	Super::PossessedBy(NewController);

	if (!CharacterStartUpData.IsNull())
	{
		if (UDataAsset_StartUpDataBase* LoadedData = CharacterStartUpData.LoadSynchronous())
		{
			LoadedData->GiveToAbilitySystemComponent(KwangAbilitySystemComponent);
		}
	}
}

void AKwangHeroCharacters::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 1. 안전장치: 블루프린트에서 '조작키 서류철(Data Asset)'을 안 끼워 넣고 게임을 실행하면, 
	// 원인을 알 수 없는 버그가 생기기 전에 여기서 즉시 에디터를 강제 종료(크래시)시키고 경고문을 띄움
	checkf(InputConfigDataAsset, TEXT("Forgot to assign a valid data asset as input config"));

	// 2. 현재 이 캐릭터를 조종하고 있는 화면 밖의 '진짜 사람(Local Player)' 정보를 가져옴
	ULocalPlayer* localPlayer = GetController<APlayerController>()->GetLocalPlayer();

	// 3. 그 사람의 입력 장치(키보드/마우스)를 관리하는 최고 관리자(Subsystem)를 호출
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(localPlayer);

	// 관리자가 정상적으로 존재하는지 확인
	check(Subsystem);

	// 4. 서류철에 들어있던 '기본 조작법(DefaultMappingContext)'을 시스템에 장착시킴
	// 숫자 0은 우선순위(Priority)
	Subsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext, 0);

	// 5. 기본 입력 컴포넌트를 우리가 개조한 '만능 어댑터 컴포넌트(UKwangInputComponent)'로 형변환(Cast)
	UKwangInputComponent* KwangInputComponent = CastChecked<UKwangInputComponent>(PlayerInputComponent);

	// 6. 키보드-함수 연결(Binding)
	// 서류철(InputConfigDataAsset)을 뒤져서 'InputTag_Move' 태그가 붙은 키보드를 누르고 있으면(Triggered), 
	// 나(this)의 Input_Move 함수를 실행
	KwangInputComponent->BindNativeInputAction(InputConfigDataAsset, KwangGameplayTags::InputTag_Move, 
		ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	KwangInputComponent->BindNativeInputAction(InputConfigDataAsset, KwangGameplayTags::InputTag_Look, 
		ETriggerEvent::Triggered, this, &ThisClass::Input_Look);

	KwangInputComponent->BindAbilityInputAction(InputConfigDataAsset, this, &ThisClass::Input_AbilityInputPressed, &ThisClass::Input_AbilityInputReleased);

}

// 키보드(WASD)를 누를 때마다 실제로 실행될 이동 콜백 함수
void AKwangHeroCharacters::Input_Move(const FInputActionValue& InputActionValue)
{
	// 1. 입력된 키보드 값을 2D 벡터(X, Y) 형태로 뽑아온다.
	// W/S를 누르면 Y축 값이 1.0 / -1.0 으로 들어오고, D/A를 누르면 X축 값이 1.0 / -1.0 으로 들어온다.
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();

	// 2. 현재 내 캐릭터를 조종하는 컨트롤러(마우스/카메라)가 바라보는 회전값을 가져온다.
	// 중요 : Pitch(위아래)와 Roll(갸우뚱)은 0.f로 무시하고, 오직 Yaw(Z축, 좌우) 값만 가져옵
	const FRotator MovementRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	// 3. 앞/뒤 이동 처리 (W, S 키)
	if (MovementVector.Y != 0.f)
	{
		// 마우스가 바라보는 방향(MovementRotation)을 기준으로 '진짜 앞쪽(Forward)'이 어딘지 수학적으로 계산
		const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);

		// 캐릭터 무브먼트 컴포넌트에게 "방금 계산한 앞쪽 방향으로, 내가 누른 힘(Y값)만큼 걸어가" 라고 명령
		AddMovementInput(ForwardDirection, MovementVector.Y);
	}

	// 4. 좌/우 이동 처리 (A, D 키)
	if (MovementVector.X != 0.f)
	{
		// 마우스가 바라보는 방향을 기준으로 '진짜 오른쪽(Right)'이 어딘지 계산
		const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);

		// 캐릭터 무브먼트 컴포넌트에게 계산된 오른쪽 방향으로 누른 힘(X값)만큼 걸어가라고 명령
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AKwangHeroCharacters::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	if (LookAxisVector.X != 0.f)
	{
		AddControllerYawInput(LookAxisVector.X);
	}

	if (LookAxisVector.Y != 0.f)
	{
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// 1.[스킬 버튼 누름 전달] 유저가 마우스 좌클릭(InputTag.Attack) 같은 스킬 버튼을 눌렀을 때 실행
void AKwangHeroCharacters::Input_AbilityInputPressed(FGameplayTag InInputTag)
{
	// [핵심 로직] 캐릭터 본체(육체)는 스킬이 어떻게 나가는지 모름. 
	// 그저 자기 머릿속의 뇌(Kwang ASC)에게 "이름표(InInputTag) 달린 버튼 눌렸으니까 네가 알아서 스킬 찾아 써!" 라고 토스
	KwangAbilitySystemComponent->OnAbilityInputPressed(InInputTag);
}

// 2. [스킬 버튼 뗌 전달] 유저가 누르고 있던 스킬 버튼에서 손을 뗐을 때 실행됨.
void AKwangHeroCharacters::Input_AbilityInputReleased(FGameplayTag InInputTag)
{
	// [핵심 로직] 마찬가지로 뇌(ASC)에게 "아까 누른 그 버튼, 방금 손 뗐다!" 라고 상황을 전달함.
	// 뇌는 이 보고를 받고 '기 모으기(차징) 스킬 이제 발사해라' 혹은 '누르고 있을 때만 나가는 방패 막기 스킬 꺼라' 같은 판단을 내리게 됨.
	KwangAbilitySystemComponent->OnAbilityInputReleased(InInputTag);
}

// Called when the game starts or when spawned
void AKwangHeroCharacters::BeginPlay()
{
	Super::BeginPlay();
}


//void AKwangHeroCharacters::PossessedBy(AController* NewController)
//{
//	 1. 부모 클래스의 기본 빙의 로직을 먼저 실행해서 뼈대 세팅
//	Super::PossessedBy(NewController);
//
//	 2. 내 캐릭터에게 뇌(ASC)와 심장(AttributeSet)이 성공적으로 생성되어 있는지 안전 검사
//	if (KwangAbilitySystemComponent && KwangAttributeSet)
//	{
//		 3. FString::Printf를 사용해 문장을 조립
//		 데이터의 진짜 주인(Owner)은 누구고, 월드에서 뛰노는 껍데기(Avatar)는 누구인가
//		const FString ASCText = FString::Printf(TEXT("Owner Actor : %s, Avartar Actor : %s"), 
//			*KwangAbilitySystemComponent->GetOwnerActor()->GetActorLabel(),	 // 데이터 주인 이름
//			*KwangAbilitySystemComponent->GetAvatarActor()->GetActorLabel()); // 물리적 껍데기 이름
//
//		 4. 화면 왼쪽 위에 초록색(FColor::Green) 글씨로 성공 메시지와 이름을 띄워줌
//		Debug::Print(TEXT("Ability system component valid ") + ASCText, FColor::Green); 
//		Debug::Print(TEXT("AttributeSet valid ") + ASCText, FColor::Green); 
//	}
//}