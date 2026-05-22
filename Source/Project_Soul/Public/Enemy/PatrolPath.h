#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "PatrolPath.generated.h"

UCLASS()
class PROJECT_SOUL_API APatrolPath : public AActor
{
	GENERATED_BODY()
	
public:	
	APatrolPath();

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol")
	USplineComponent* Spline;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	FVector GetPatrolPoint(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	int32 GetPatrolPointCount() const;
};