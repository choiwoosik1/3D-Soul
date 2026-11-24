// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/HeroGameplayAbility_TargetLock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Characters/KwangHeroCharacters.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/KwangWidgetBase.h"
#include "KwangHeroController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "KwangFunctionLibrary.h"
#include "KwangGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "Enemy/Enemy.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include "KwangDebugHelper.h"

void UHeroGameplayAbility_TargetLock::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (AKwangHeroCharacters* Hero = GetHeroCharacterFromActorInfo())
	{
		if (USpringArmComponent* Boom = Hero->GetCameraBoom())
		{
			DefaultArmLength = Boom->TargetArmLength;
		}
	}

	TryLockOnTarget();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UHeroGameplayAbility_TargetLock::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CleanUp();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UHeroGameplayAbility_TargetLock::OnTargetLockTick(float DeltaTime)
{
	// 플레이어가 죽었는지 확인
	if (UKwangFunctionLibrary::NativeDoesActorHaveTag(          // 태그 확인 함수
		GetHeroCharacterFromActorInfo(),                        // 확인 대상: 플레이어
		KwangGameplayTags::Shared_Status_Dead))                 // "죽음" 태그가 붙어 있으면 true
	{
		CancelTargetLockAbility();   // 락온 어빌리티 종료. EndAbility가 불려서 CleanUp()까지 실행
		return;                      // 
	}

	// 락온한 적을 잃었는지 확인
	if (CalculateThreatScore(CurrentLockedActor) < 0.f)   // 점수 -1이면 "죽음, null, 락온 거리 초과" 중 하나
	{
		if (!TrySwitchToNextTarget())   // 다음 적 찾기. 못 찾으면 false
		{
			CancelTargetLockAbility();  // 근처에 적이 없으면 락온 해제
			return;
		}
		// 찾았으면 CurrentLockedActor가 새 적으로 바뀐 채로 아래 Tick 로직이 계속 실행
	}

	// 대상이 너무 멀어지면 락온 해제
	const float DistanceToTarget = FVector::Dist(
		GetHeroCharacterFromActorInfo()->GetActorLocation(),
		CurrentLockedActor->GetActorLocation());

	if (DistanceToTarget > MaxTargetLockDistance)
	{
		CancelTargetLockAbility();
		return;
	}

	// Drop the lock when the target stays hidden behind a wall
	if (HasLineOfSightTo(CurrentLockedActor))
	{
		LostSightTimer = 0.f;
	}
	else
	{
		LostSightTimer += DeltaTime;
		if (LostSightTimer >= LoseSightTime)
		{
			LostSightTimer = 0.f;
			if (!TrySwitchToNextTarget())
			{
				CancelTargetLockAbility();
				return;
			}
		}
	}

	SetTargetLockWidgetPosition();

	TryRetargetByThreat(DeltaTime);

	const bool bIsRolling =
		UKwangFunctionLibrary::NativeDoesActorHaveTag(GetHeroCharacterFromActorInfo(), KwangGameplayTags::Player_Status_Rolling) &&
		!UKwangFunctionLibrary::NativeDoesActorHaveTag(GetHeroCharacterFromActorInfo(), KwangGameplayTags::Player_Status_Rolling_Strafe);
	const bool bIsBlocking = UKwangFunctionLibrary::NativeDoesActorHaveTag(GetHeroCharacterFromActorInfo(), KwangGameplayTags::Player_Status_Blocking);

	// 구르는 중에는 회전 간섭 안 함
	if (!bIsRolling)
	{
		FVector TargetLocation = CurrentLockedActor->GetActorLocation() + FVector(0.f, 0.f, 80.f);
		FVector StartLocation = GetHeroCharacterFromActorInfo()->GetActorLocation() + FVector(0.f, 0.f, 50.f);

		FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);

		// Close range: pull the pitch toward ClosePitch so the camera does not tilt up
		const float FlatDistance = FVector::Dist2D(StartLocation, TargetLocation);
		const float CloseAlpha = FMath::Clamp(1.f - (FlatDistance / LockCloseDistance), 0.f, 1.f);

		// Close range: pull the pitch toward ClosePitch so the camera does not tilt up
		LookAtRot.Pitch = FMath::Lerp(LookAtRot.Pitch, ClosePitch, CloseAlpha);
		LookAtRot.Pitch = FMath::Clamp(LookAtRot.Pitch, MinLockPitch, MaxLockPitch);

		// Close range: move the camera back to see the target
		if (USpringArmComponent* Boom = GetHeroCharacterFromActorInfo()->GetCameraBoom())
		{
			const float GoalLength = FMath::Lerp(DefaultArmLength, ClosePullBackLength, CloseAlpha);
			Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, GoalLength, DeltaTime, ArmLengthInterpSpeed);
		}

		const FRotator CurrentControlRot = GetHeroControllerFromActorInfo()->GetControlRotation();
		const FRotator TargetRot = FMath::RInterpTo(CurrentControlRot, LookAtRot, DeltaTime, TargetLockRotationInterpSpeed);

		// 카메라는 방어 중에도 락온 대상을 따라감
		GetHeroControllerFromActorInfo()->SetControlRotation(FRotator(TargetRot.Pitch, TargetRot.Yaw, 0.f));

		// 몸 회전은 방어 중이 아닐 때만 (방어 중에는 Tick의 UpdateBlockRotation이 담당)
		if (!bIsBlocking)
		{
			GetHeroCharacterFromActorInfo()->SetActorRotation(FRotator(0.f, TargetRot.Yaw, 0.f));
		}
	}
}

void UHeroGameplayAbility_TargetLock::TryLockOnTarget()
{
	GetAvaillableActorsToLock();

	if (AvailableActorsToLock.IsEmpty())
	{
		CancelTargetLockAbility();
		return;
	}
	CurrentLockedActor = GetBestTargetFromAvailableActors(AvailableActorsToLock);

	GetHeroCharacterFromActorInfo()->SetLockedTarget(CurrentLockedActor);

	if (CurrentLockedActor)
	{
		DrawTargetLockAbility();

		SetTargetLockWidgetPosition();
	}
	else
	{
		CancelTargetLockAbility();
	}

}

void UHeroGameplayAbility_TargetLock::GetAvaillableActorsToLock()
{
	TArray<FHitResult> BoxTraceHits;

	UKismetSystemLibrary::BoxTraceMultiForObjects(
		GetHeroCharacterFromActorInfo(),
		GetHeroCharacterFromActorInfo()->GetActorLocation(),
		GetHeroCharacterFromActorInfo()->GetActorLocation() + GetHeroCharacterFromActorInfo()->GetActorForwardVector() * BoxTraceDistance,
		TraceBoxSize / 2.f,
		GetHeroCharacterFromActorInfo()->GetActorForwardVector().ToOrientationRotator(),
		BoxTraceChannel,
		false,
		TArray<AActor*>(),
		bShowPersistentDebugShape ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None,
		BoxTraceHits,
		true
	);

	for (const FHitResult& TraceHit : BoxTraceHits)
	{
		if (AActor* HitActor = TraceHit.GetActor())
		{
			if (HitActor != GetHeroCharacterFromActorInfo() && HasLineOfSightTo(HitActor))
			{
				AvailableActorsToLock.AddUnique(HitActor);

				/*Debug::Print(HitActor->GetActorNameOrLabel());*/
			}
		}
	}
}

AActor* UHeroGameplayAbility_TargetLock::GetBestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors)
{
	AActor* BestTarget = nullptr;
	float BestScore = -1.f;

	for (AActor* Candidate : InAvailableActors)
	{
		const float Score = CalculateThreatScore(Candidate);

		//UE_LOG(LogTemp, Verbose, TEXT("[Threat] %s score = %.2f"),
			//Candidate ? *Candidate->GetName() : TEXT("null"), Score);

		// Excluded candidates return -1, so they never beat BestScore
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

float UHeroGameplayAbility_TargetLock::CalculateThreatScore(const AActor* Candidate)
{
	if (!Candidate)
	{
		return -1.f;
	}

	// Never pick dead targets
	if (UKwangFunctionLibrary::NativeDoesActorHaveTag(const_cast<AActor*>(Candidate), KwangGameplayTags::Shared_Status_Dead))
	{
		return -1.f;
	}

	const AEnemy* Enemy = Cast<AEnemy>(Candidate);
	if (Enemy && Enemy->GetCharacterState() == EEnemyState::Dead)
	{
		return -1.f;
	}

	const FVector HeroLocation = GetHeroCharacterFromActorInfo()->GetActorLocation();
	const float Distance = FVector::Dist(HeroLocation, Candidate->GetActorLocation());

	// Beyond the lock range the lock would be cancelled right away, so skip
	if (Distance > MaxTargetLockDistance)
	{
		return -1.f;
	}

	// 1) Distance: closer = higher (0..1)
	const float DistanceScore = FMath::Clamp(1.f - Distance / MaxTargetLockDistance, 0.f, 1.f);

	// 2) Angle: how close to the camera forward direction (0..1, behind = 0)
	const FVector CameraForward = GetHeroControllerFromActorInfo()->GetControlRotation().Vector().GetSafeNormal2D();
	const FVector ToCandidate = (Candidate->GetActorLocation() - HeroLocation).GetSafeNormal2D();
	const float AngleScore = FMath::Clamp(FVector::DotProduct(CameraForward, ToCandidate), 0.f, 1.f);

	float Score = DistanceScore * DistanceWeight + AngleScore * AngleWeight;

	// 3) Threat: enemy is attacking right now
	if (Enemy && Enemy->GetCharacterState() == EEnemyState::Attack)
	{
		const bool bIsBlocking = UKwangFunctionLibrary::NativeDoesActorHaveTag(
			GetHeroCharacterFromActorInfo(), KwangGameplayTags::Player_Status_Blocking);

		Score += AttackingWeight * (bIsBlocking ? BlockingAttackMultiplier : 1.f);

		if (Enemy->CanBeParried())
		{
			Score += ParryableBonus;
		}
	}

	return Score;
}

void UHeroGameplayAbility_TargetLock::DrawTargetLockAbility()
{
	if (!DrawnTargetLockWidget)
	{
		checkf(TargetLockWidgetClass, TEXT("Forgot to assifn a valid widget class in Blueprint"));

		DrawnTargetLockWidget = CreateWidget<UKwangWidgetBase>(GetHeroControllerFromActorInfo(), TargetLockWidgetClass);

		check(DrawnTargetLockWidget);

		DrawnTargetLockWidget->AddToViewport();
	}
}

void UHeroGameplayAbility_TargetLock::SetTargetLockWidgetPosition()
{
	if (!DrawnTargetLockWidget || !CurrentLockedActor)
	{
		CancelTargetLockAbility();
		return;
	}

	FVector2D ScreenPosition;
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		GetHeroControllerFromActorInfo(),
		CurrentLockedActor->GetActorLocation(),
		ScreenPosition,
		true
	);

	if (TargetLockWidgetSize == FVector2D::ZeroVector)
	{
		DrawnTargetLockWidget->WidgetTree->ForEachWidget(
			[this](UWidget* FoundWidget)
			{
				if (USizeBox* FoundSizeBox = Cast<USizeBox>(FoundWidget))
				{
					TargetLockWidgetSize.X = FoundSizeBox->GetWidthOverride();
					TargetLockWidgetSize.Y = FoundSizeBox->GetHeightOverride();
				}
			}
		);
	}

	ScreenPosition -= (TargetLockWidgetSize / 2.f);

	DrawnTargetLockWidget->SetPositionInViewport(ScreenPosition, false);
}

void UHeroGameplayAbility_TargetLock::CancelTargetLockAbility()
{
	CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UHeroGameplayAbility_TargetLock::CleanUp()
{
	AvailableActorsToLock.Empty();

	RetargetTimer = 0.f;
	LostSightTimer = 0.f;
	RetargetCooldownRemaining = 0.f;

	if (AKwangHeroCharacters* HeroForCamera = GetHeroCharacterFromActorInfo())
	{
		if (USpringArmComponent* Boom = HeroForCamera->GetCameraBoom())
		{
			if (DefaultArmLength > 0.f)
			{
				Boom->TargetArmLength = DefaultArmLength;
			}
		}
	}

	CurrentLockedActor = nullptr;

	if (AKwangHeroCharacters* Hero = GetHeroCharacterFromActorInfo()) { Hero->SetLockedTarget(nullptr); }

	if (DrawnTargetLockWidget)
	{
		DrawnTargetLockWidget->RemoveFromParent();
	}
	DrawnTargetLockWidget = nullptr;

	TargetLockWidgetSize = FVector2D::ZeroVector;
}

void UHeroGameplayAbility_TargetLock::TryRetargetByThreat(float DeltaTime)
{
	if (!bAutoRetarget)
	{
		return;
	}

	RetargetCooldownRemaining = FMath::Max(0.f, RetargetCooldownRemaining - DeltaTime);

	// Re-evaluate only every RetargetInterval seconds
	RetargetTimer += DeltaTime;
	if (RetargetTimer < RetargetInterval)
	{
		return;
	}
	RetargetTimer = 0.f;

	if (RetargetCooldownRemaining > 0.f)
	{
		return;
	}

	AKwangHeroCharacters* Hero = GetHeroCharacterFromActorInfo();

	// Do not swing the camera to a new target in the middle of a roll
	if (UKwangFunctionLibrary::NativeDoesActorHaveTag(Hero, KwangGameplayTags::Player_Status_Rolling))
	{
		return;
	}

	const bool bIsBlocking = UKwangFunctionLibrary::NativeDoesActorHaveTag(Hero, KwangGameplayTags::Player_Status_Blocking);
	if (bRetargetOnlyWhileBlocking && !bIsBlocking)
	{
		return;
	}

	// Refresh the candidate list (the old list was built when the lock started)
	AvailableActorsToLock.Empty();
	GetAvaillableActorsToLock();

	// No candidates or same target: keep the current lock
	AActor* BestTarget = GetBestTargetFromAvailableActors(AvailableActorsToLock);
	if (!BestTarget || BestTarget == CurrentLockedActor)
	{
		return;
	}

	const float CurrentScore = CalculateThreatScore(CurrentLockedActor);
	const float BestScore = CalculateThreatScore(BestTarget);

	// Switch only if the new target is clearly more threatening
	if (BestScore > CurrentScore + RetargetMargin)
	{
		CurrentLockedActor = BestTarget;
		Hero->SetLockedTarget(CurrentLockedActor);
		RetargetCooldownRemaining = RetargetCooldown;
	}
}

void UHeroGameplayAbility_TargetLock::GetNearbyActorsToLock()
{
	AKwangHeroCharacters* Hero = GetHeroCharacterFromActorInfo();   // 플레이어. 아래에서 여러 번 써서 변수로 뺌

	TArray<AActor*> OverlappedActors;   // 검색 결과를 받을 배열
	TArray<AActor*> ActorsToIgnore;     // 검색에서 제외할 대상 목록
	ActorsToIgnore.Add(Hero);           // 플레이어 자신은 후보에서 제외

	UKismetSystemLibrary::SphereOverlapActors(   // 구 모양 범위에 겹치는 액터 검색
		Hero,                                    // 월드를 알기 위한 기준 오브젝트
		Hero->GetActorLocation(),                // 구의 중심: 플레이어 위치
		NextTargetSearchRadius,                  // 구의 반지름 (기본 1500)
		BoxTraceChannel,                         // 검색할 오브젝트 타입. 박스 트레이스와 같은 값
		AActor::StaticClass(),                   // 모든 액터 클래스 허용
		ActorsToIgnore,                          // 제외 목록
		OverlappedActors                         // 결과가 여기에 담김
	);

	for (AActor* Found : OverlappedActors)       // 찾은 액터를 하나씩 순회
	{
		if (HasLineOfSightTo(Found))
		{
			AvailableActorsToLock.AddUnique(Found);
		}
	}
}

bool UHeroGameplayAbility_TargetLock::TrySwitchToNextTarget()
{
	AvailableActorsToLock.Empty();   // 예전에 모은 후보는 낡았으니 비움
	GetNearbyActorsToLock();         // 주변 후보 다시 수집

	AActor* NextTarget = GetBestTargetFromAvailableActors(AvailableActorsToLock);   // 점수 1등을 뽑음
	if (!NextTarget)                 // 한 명도 못 찾았으면
	{
		return false;                // "다음 적 없음"을 알려서 호출한 쪽이 락온을 풀게 함
	}

	CurrentLockedActor = NextTarget; // 락온 대상 교체

	GetHeroCharacterFromActorInfo()->SetLockedTarget(CurrentLockedActor);

	RetargetTimer = 0.f;                          // 주기적 재선택 타이머를 처음부터 다시 셈
	RetargetCooldownRemaining = RetargetCooldown; // 쿨다운을 새로 걸어서, 전환 직후 다시 바뀌는 걸 막음

	return true;                     // "교체 성공"
}

bool UHeroGameplayAbility_TargetLock::HasLineOfSightTo(const AActor* Target)
{
	AKwangHeroCharacters* Hero = GetHeroCharacterFromActorInfo();
	if (!Hero || !Target)
	{
		return false;
	}

	// Aim at the head: capsule center + 80% of the capsule half height (scales with big enemies)
	auto GetHeadLocation = [](const AActor* Actor)
		{
			FVector Location = Actor->GetActorLocation();
			if (const ACharacter* Character = Cast<ACharacter>(Actor))
			{
				Location.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.8f;
			}
			return Location;
		};

	const FVector Start = GetHeadLocation(Hero);
	const FVector End = GetHeadLocation(Target);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TargetLockLOS), false, Hero);
	Params.AddIgnoredActor(Target);

	TArray<AActor*> AttachedToHero;
	Hero->GetAttachedActors(AttachedToHero);
	Params.AddIgnoredActors(AttachedToHero);

	// Other characters standing in between do not hide the target, so skip them and trace again
	for (int32 i = 0; i < 4; ++i)
	{
		FHitResult Hit;
		if (!Hero->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, LineOfSightChannel, Params))
		{
			return true;
		}

		if (const APawn* BlockingPawn = Cast<APawn>(Hit.GetActor()))
		{
			Params.AddIgnoredActor(BlockingPawn);
			continue;
		}
		return false;
	}
	return true;
}