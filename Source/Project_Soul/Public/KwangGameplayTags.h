// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace KwangGameplayTags
{
	// 해당 매크로는 다른 모듈에서 이러한 게임 플레이 태그의 접근성을 보장
	/** Input Tags **/
	PROJECT_SOUL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	PROJECT_SOUL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look);	
	PROJECT_SOUL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_LightAttack_Sword);
	PROJECT_SOUL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_HeavyAttack_Sword);

	/** Player Tags **/
	PROJECT_SOUL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ability_Attack_Light_Sword);
	PROJECT_SOUL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Ability_Attack_Heavy_Sword);

	PROJECT_SOUL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Weapon_Sword);
}