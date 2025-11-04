#include "BreakActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

ABreakActor::ABreakActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // ✅ Meshをルートにする（乗る部分）
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Mesh->SetGenerateOverlapEvents(false); // Block専用
    Mesh->SetSimulatePhysics(false);

    // ✅ Overlap用の透明Box（少し大きめに）
    OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
    OverlapBox->SetupAttachment(Mesh);
    OverlapBox->SetBoxExtent(FVector(55.f, 55.f, 15.f));
    OverlapBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OverlapBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    OverlapBox->SetGenerateOverlapEvents(true);
}

void ABreakActor::BeginPlay()
{
    Super::BeginPlay();

    OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &ABreakActor::OnOverlapBegin);
}

void ABreakActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Warning, TEXT("Overlap detected with %s"), OtherActor ? *OtherActor->GetName() : TEXT("None"));

    if (!bIsBroken && OtherActor && OtherActor->ActorHasTag(TEXT("Player")))
    {
        UE_LOG(LogTemp, Warning, TEXT("Player overlapped! Scheduling break..."));

        // タイマー設定（1秒後に壊す）
        GetWorldTimerManager().SetTimer(
            BreakTimerHandle,
            FTimerDelegate::CreateLambda([this]()
                {
                    UE_LOG(LogTemp, Warning, TEXT("BreakPlatform timer triggered"));
                    BreakPlatform();
                }),
            BreakDelay,
            false
        );
    }
}

void ABreakActor::BreakPlatform()
{
    if (bIsBroken) return;
    bIsBroken = true;

    UE_LOG(LogTemp, Warning, TEXT("BreakPlatform called!"));

    Mesh->SetVisibility(false);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OverlapBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ABreakActor::RespawnPlatform, RespawnDelay, false);
}

void ABreakActor::RespawnPlatform()
{
    bIsBroken = false;

    Mesh->SetVisibility(true);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    OverlapBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}
