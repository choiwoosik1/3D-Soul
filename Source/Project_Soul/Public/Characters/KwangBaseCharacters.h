// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "KwangBaseCharacters.generated.h"

class UKwangAbilitySystemComponent;
class UKwangAttributeSet;
class UDataAsset_StartUpDataBase;

UCLASS()
class PROJECT_SOUL_API AKwangBaseCharacters : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AKwangBaseCharacters();
	//~ Begin IAbilitySystemInterface Interface.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const;
	//~ End IAbilitySystemInterface Interface

protected:
	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	UKwangAbilitySystemComponent* KwangAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	UKwangAttributeSet* KwangAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	TSoftObjectPtr<UDataAsset_StartUpDataBase> CharacterStartUpData;
	
public:
	FORCEINLINE UKwangAbilitySystemComponent* GetKwangAbilitySystemComponent() const { return KwangAbilitySystemComponent; }
	FORCEINLINE UKwangAttributeSet* GetKwangAttributeSet() const { return KwangAttributeSet; }
};
