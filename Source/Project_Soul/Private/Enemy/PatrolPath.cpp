#include "Enemy/PatrolPath.h"

APatrolPath::APatrolPath()
{
    Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
    RootComponent = Spline;
}

FVector APatrolPath::GetPatrolPoint(int32 Index) const
{
    if (!Spline) return FVector::ZeroVector;
    return Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
}

int32 APatrolPath::GetPatrolPointCount() const
{
    if (!Spline) return 0;
    return Spline->GetNumberOfSplinePoints();
}