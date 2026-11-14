// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Characters/KwangBaseCharacters.h"
#include "GameplayTagContainer.h"
#include "KwangHeroCharacters.generated.h"

// 전방 선언 (Forward Declaration): 컴파일 속도 향상을 위해 포인터 크기만 임시로 확보
class USpringArmComponent;
class UCameraComponent;
class UDataAsset_InputConfig;
struct FInputActionValue;
class UHeroCombatComponent;
class UHeroUIComponent;
class UGameplayEffect;
class ACheckpoint;

UCLASS()
class PROJECT_SOUL_API AKwangHeroCharacters : public AKwangBaseCharacters
{
	GENERATED_BODY()

public:
	AKwangHeroCharacters();

	//~ Begin IPawnCombatInterfaceInterface.
	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
	//~ End IPawnCombatInterfaceInterface.

	//~ Begin IPawnUIInterface Interface.
	virtual UPawnUIComponent* GetPawnUIComponent() const override;
	virtual UHeroUIComponent* GetHeroUIComponent() const override;
	//~ End IPawnUIInterface Interface.

protected:
	// [언리얼 특수 주석] 이 안쪽은 'APawn'이라는 조상 클래스가 물려준 기능들을 모아둔 곳이라고 표시
	// 나중에 언리얼 공식 문서처럼 예쁜 API 설명서를 자동 생성할 때 쓰이는 기능
	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interface

	// 1. 입력 바인딩 전용 함수: 플레이어가 이 캐릭터를 조종(Possess)하게 될 때 엔진이 자동으로 호출해 주는 함수
	// 이 안에서 "W키 누르면 앞으로 가라" 같은 케이블 연결 작업
	// override : 현재 클래스에서 새로 만든 게 아닌 부모 클래스 함수 덮어쓰기.
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// Called when the game starts or when spawned
	// virtual : 부모 클래스의 원본 함수가 있지만 현재 클래스 상황에 맞게 고침
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	// Landing makes noise (AI hearing)
	virtual void Landed(const FHitResult& Hit) override;

private:
	// #pragma region: Visual Studio에서 코드를 접었다 펼칠 수 있게 해주는 단순 UI 기능
#pragma region Components

	// VisibleAnywhere: 에디터 디테일 패널에서 컴포넌트 속성을 볼 수 있게 함
	// BlueprintReadOnly: 블루프린트 노드에서 이 변수를 읽을 수만 있게 함 (덮어쓰기 방지)
	// meta=(AllowPrivateAccess="true"): C++에서는 private(외부 접근 불가)으로 안전하게 잠가두되, 에디터(블루프린트)에서는 예외적으로 접근할 수 있게 허용하는 옵션
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UHeroCombatComponent* HeroCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	UHeroUIComponent* HeroUIComponent;

#pragma endregion

#pragma region Inputs
	// EditDefaultsOnly: 블루프린트 에디터 창에서 우리가 만든 '조작키 서류철(.asset)'을 꽂아 넣을 수 있게 해줌
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	UDataAsset_InputConfig* InputConfigDataAsset;

	// 3. 실제 이동 로직이 담길 콜백(Callback) 함수
	// 키보드(WASD)나 조이스틱을 움직였을 때 그 '입력된 값(앞으로 1, 옆으로 -1 등)'을 받아와서 캐릭터를 움직이게 함
	void Input_Move(const FInputActionValue& InputActionValue);

	void Input_Look(const FInputActionValue& InputActionValue);

	// 5. [스킬 버튼 누름 콜백] 유저가 공격이나 구르기 버튼을 '누르는 순간' 호출됨.
	// 입력 시스템이 "InputTag.Attack 눌렸어!" 하고 태그(InInputTag)를 여기로 던져주면, 여기서 GAS(뇌)에게 스킬 발동을 명령
	void Input_AbilityInputPressed(FGameplayTag InInputTag);

	// 6.[스킬 버튼 뗌 콜백] 유저가 누르고 있던 스킬 버튼에서 '손을 떼는 순간' 호출됨.
	// '차징 스킬(꾹 모았다가 쏘는 스킬)' 같은 걸 구현
	void Input_AbilityInputReleased(FGameplayTag InInputTag);
#pragma endregion

#pragma region Combat
public:
	bool IsBlocking() const;

	// 적 AI가 플레이어 상태를 읽는 용도
	UFUNCTION(BlueprintPure, Category = "Combat|State")
	bool IsGuarding() const;     // 가드 시작 ~ 가드 브레이크 피해 적용 전

	UFUNCTION(BlueprintPure, Category = "Combat|State")
	bool IsAttacking() const;    // 공격 시작 ~ 후딜 끝

	UFUNCTION(BlueprintPure, Category = "Combat|State")
	bool IsRolling() const;      // 구르기 전체

	UFUNCTION(BlueprintPure, Category = "Combat|State")
	bool IsInvincible() const { return bIsInvincible; } // 구르기 무적 프레임

public:
	FORCEINLINE UHeroCombatComponent* GetHeroCombatComponent() const { return HeroCombatComponent; }

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	void TriggerGuardBreak(bool bIsHeavyBreak);

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Block으로 인해 스태미나를 줄이는데 사용할 GameEffect
	UPROPERTY(EditDefaultsOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> BlockStaminaCostEffectClass;

	// 막은 데미지 스태미나 줄이는 배율
	UPROPERTY(EditAnywhere, Category = "Block")
	float BlockStaminaCostRatio = 1.0f;

	// 패링 성공시 스태미나 줄이는 배율
	UPROPERTY(EditAnywhere, Category = "Block")
	float ParryStaminaCostMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> GuardBrokenEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> HeavyGuardBrokenEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GuardBreak")
	TSubclassOf<UGameplayEffect> RageResetEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Block")
	UAnimMontage* GuardBrokenMontage;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetInvincible(bool bInvincible);

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	UAnimMontage* DeathMontage;


#pragma region Checkpoint
public:
	// Called by ACheckpoint when the player interacts with it (E key): remembers it and restores stats
	void RestAtCheckpoint(ACheckpoint* Checkpoint);

	// Called when the death sequence finishes: undoes the death state, restores stats, moves to the last checkpoint
	void Respawn();

	// Called when the hero dies (KwangAttributeSet): calls Respawn() after the death montage + RespawnDelay
	void ScheduleRespawn();

	// Extra wait (seconds) after the death montage before respawning
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnDelay = 3.f;
#pragma endregion

#pragma region Noise
public:
	// Reports a noise event to the AI hearing sense. Loudness scales the enemies' hearing range,
	// MaxRange 0 = no extra limit. Also callable from Blueprints / abilities.
	UFUNCTION(BlueprintCallable, Category = "Noise")
	void MakePlayerNoise(float Loudness, FName NoiseTag = NAME_None, float MaxRange = 0.f);

	// ---- Noise tuning (designer values, tweak freely) ----
	UPROPERTY(EditAnywhere, Category = "Noise")
	float FootstepLoudness = 0.5f;

	// Seconds between footsteps while moving at full walk speed
	UPROPERTY(EditAnywhere, Category = "Noise")
	float FootstepInterval = 0.45f;

	// No footstep noise below this speed (cm/s)
	UPROPERTY(EditAnywhere, Category = "Noise")
	float MinFootstepSpeed = 50.f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float LandingLoudness = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise")
	float AttackLoudness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	float RollLoudness = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noise")
	float SkillLoudness = 1.2f;

	// 0 = unlimited (the enemy's own HearingRange still applies)
	UPROPERTY(EditAnywhere, Category = "Noise")
	float NoiseMaxRange = 0.f;

	// Remaining time / total duration of a skill cooldown (for the HUD). Returns true while on cooldown.
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool GetSkillCoolDown(FGameplayTag CooldownTag, float& RemainingTime, float& TotalDuration) const;
#pragma endregion

private:
	bool bIsInvincible = false;
	FTimerHandle GuardBrokenMontageTimerHandle;

	void UpdateBlockRotation(float DeltaTime);
	AActor* FindNearestEnemy(float SearchRadius) const;

	UPROPERTY(EditAnywhere, Category = "Block")
	float BlockRotationInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Block")
	float BlockEnemySearchRadius = 1500.f;

	// ---- Checkpoint ----
	void RestoreStats();

	TWeakObjectPtr<ACheckpoint> LastCheckpoint;

	// Fallback respawn position when no checkpoint has been used yet
	FTransform InitialTransform;

	FTimerHandle RespawnTimerHandle;

	// ---- Noise ----
	void UpdateFootstepNoise(float DeltaTime);
	void NotifyAbilityNoise(const FGameplayTag& InInputTag);

	float FootstepTimer = 0.f;
};
#pragma endregion

