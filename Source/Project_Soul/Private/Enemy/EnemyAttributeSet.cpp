#include "Enemy/EnemyAttributeSet.h"
#include "Enemy/Enemy.h"
#include "Interfaces/PawnUIInterface.h"
#include "Components/UI/EnemyUIComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"

UEnemyAttributeSet::UEnemyAttributeSet()
{
    InitDamageTaken(0.f);
    InitAttackPower(1.f);
    InitDefensePower(1.f);
}

void UEnemyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetDamageTakenAttribute())
    {
        float Damage = GetDamageTaken();
        SetDamageTaken(0.f);

        if (Damage > 0.f)
        {
            AEnemy* Enemy = Cast<AEnemy>(GetOwningActor());
            if (Enemy)
            {
				// Apply damage to the enemy character
				APawn* InstigatorPawn = Cast<APawn>(Data.EffectSpec.GetContext().GetInstigator());
				AController* Instigator = InstigatorPawn ? InstigatorPawn->GetController() : nullptr;
				AActor* Causer = const_cast<AActor*>(Data.EffectSpec.GetContext().GetEffectCauser());

				Enemy->TakeDamage(Damage, FDamageEvent(), Instigator, Causer);

                if (IPawnUIInterface* UIInterface = Cast<IPawnUIInterface>(Enemy))
                {
                    if (UEnemyUIComponent* UIComp = Cast<UEnemyUIComponent>(UIInterface->GetPawnUIComponent()))
                    {
						UIComp->OnCurrentHealthChanged.Broadcast(Enemy->GetCurrentHealth() / Enemy->GetMaxHealth());
                    }
				}
            }
        }
    }
}