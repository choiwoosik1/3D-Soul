// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "KwangTypes/KwangStructTypes.h"
#include "DataAsset_HeroStartUpData.generated.h"


/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API UDataAsset_HeroStartUpData : public UDataAsset_StartUpDataBase
{
	GENERATED_BODY()
	
public:
	// 2. [스킬 부여 실행 함수] 이 데이터 애셋에 적힌 스킬 목록을 캐릭터의 뇌(ASC)에 실제로 주입해 주는 함수
	virtual void GiveToAbilitySystemComponent(UKwangAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1) override;
private:
	// 3. [영웅 전용 스킬 배열] 위에서 만든 [스킬+버튼 바구니]를 여러 개 담을 수 있는 리스트.
	// 'Hero Start Up Ability Sets' 메뉴가 바로 이 변수
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData", meta = (TitleProperty = "InputTag"))
	TArray<FKwangHeroAbilitySet> HeroStartUpAbilitySets;
};
