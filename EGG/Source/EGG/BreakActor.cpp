#include "BreakActor.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

ABreakActor::ABreakActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    Mesh->SetSimulatePhysics(false);
    Mesh->SetNotifyRigidBodyCollision(true);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ABreakActor::BeginPlay()
{
    Super::BeginPlay();
    Mesh->OnComponentHit.AddDynamic(this, &ABreakActor::OnHit);
}

void ABreakActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!bIsBroken && OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
    {
        GetWorld()->GetTimerManager().SetTimer(BreakTimerHandle, this, &ABreakActor::BreakPlatform, BreakDelay, false);
    }
}

void ABreakActor::BreakPlatform()
{
    if (bIsBroken) return;
    bIsBroken = true;

    Mesh->SetVisibility(false);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &ABreakActor::RespawnPlatform, RespawnDelay, false);
}

void ABreakActor::RespawnPlatform()
{
    bIsBroken = false;
    Mesh->SetVisibility(true);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
