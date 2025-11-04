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

    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* OverlapBox;

    UPROPERTY(EditAnywhere)
    float BreakDelay = 1.0f;

    UPROPERTY(EditAnywhere)
    float RespawnDelay = 3.0f;

    bool bIsBroken = false;

    FTimerHandle BreakTimerHandle;
    FTimerHandle RespawnTimerHandle;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void BreakPlatform();
    void RespawnPlatform();
};
