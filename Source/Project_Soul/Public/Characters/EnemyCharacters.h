// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/KwangBaseCharacters.h"
#include "EnemyCharacters.generated.h"

class UEnemyCombatComponent;
/**
 * 
 */
UCLASS()
class PROJECT_SOUL_API AEnemyCharacters : public AKwangBaseCharacters
{
	GENERATED_BODY()
	
public:
	AEnemyCharacters();

	//~ Begin IPawnCombatInterfaceInterface.
	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
	//~ End IPawnCombatInterfaceInterface.

protected:
	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UEnemyCombatComponent* EnemyCombatComponent;

private:
	void InitEnemyStartUpData();

public:
	FORCEINLINE UEnemyCombatComponent* GetEnemyCombatComponent() const { return EnemyCombatComponent; }
};
