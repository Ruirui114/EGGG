#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreakActor.generated.h"

UCLASS()
class EGG_API ABreakActor : public AActor
{
    GENERATED_BODY()

public:
    ABreakActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

    UPROPERTY(EditAnywhere)
    float BreakDelay = 1.0f;

    UPROPERTY(EditAnywhere)
    float RespawnDelay = 3.0f;

    bool bIsBroken = false;
    FTimerHandle BreakTimerHandle;
    FTimerHandle RespawnTimerHandle;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit);

    void BreakPlatform();
    void RespawnPlatform();
};
