#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "EnemyDamageTypes.generated.h"

// Damage type for blockable attacks 
// Player can block and reduce damage, but stamina is consumed
// If perfect guard, the player takes no damage and less stamina is consumed
UCLASS()
class PROJECT_SOUL_API UDamageType_Block : public UDamageType
{
    GENERATED_BODY()
};

// Damage type for guard break attacks
// Player can block and reduce damage, but stamina drops to 0 and guard breaks
// If perfect guard, the player takes no damage but stamina is consumed
UCLASS()
class PROJECT_SOUL_API UDamageType_GuardBreak : public UDamageType
{
    GENERATED_BODY()
};

// Damage type for block ignore attacks
// Player cannot block and takes full damage, stamina is consumed
UCLASS()
class PROJECT_SOUL_API UDamageType_Ignore : public UDamageType
{
    GENERATED_BODY()
};