// Fill out your copyright notice in the Description page of Project Settings.


#include "EggPlayer.h"

#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h" 
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
// Sets default values
AEggPlayer::AEggPlayer()
{

	// StaticMeshComponentを追加し、RootComponentに設定する
	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = Sphere;

	// StaticMeshをLaodしてStaticMeshComponentのStaticMeshに設定する
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));

	// StaticMeshをStaticMeshComponentに設定する
	Sphere->SetStaticMesh(Mesh);

	// MaterialをStaticMeshに設定する
	UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));

	// MaterialをStaticMeshComponentに設定する
	Sphere->SetMaterial(0, Material);

	// Simulate Physicsを有効にする
	Sphere->SetSimulatePhysics(true);

	// Hit Eventを有効にする
	Sphere->BodyInstance.bNotifyRigidBodyCollision = true;

	// SpringArmを追加する
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArm->SetupAttachment(RootComponent);

	// Spring Armの長さを調整する
	SpringArm->TargetArmLength = 450.0f;

	// PawnのControllerRotationを使用する
	SpringArm->bUsePawnControlRotation = true;

	// CameraのLagを有効にする
	SpringArm->bEnableCameraLag = true;

	// Cameraを追加する
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	Camera->SetupAttachment(SpringArm);

	FireCollision = CreateDefaultSubobject<USphereComponent>(TEXT("FireCollision"));
	FireCollision->SetupAttachment(Sphere);
	FireCollision->SetSphereRadius(100.f); // 炎の範囲に合わせる
	FireCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 初期は無効
	FireCollision->SetCollisionResponseToAllChannels(ECR_Overlap);

	// MotionBlurをオフにする
	Camera->PostProcessSettings.MotionBlurAmount = 0.0f;

	// Input Mapping Context「IM_Controls」を読み込む
	DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/PlayerInput"));

	// Input Action「IA_Control」を読み込む
	ControlAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Control"));

	// Input Action「IA_Look」を読み込む
	LookAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Look"));

	// Input Action「IA_Jump」を読み込む
	JumpAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Jump"));

	// Input Action「IA_Boost」を読み込む
	BoostAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Boost"));

	// デフォルト値
	bIsGoalReached = false;

}

// Called when the game starts or when spawned
void AEggPlayer::BeginPlay()
{
	Super::BeginPlay();

	//Add Input Mapping Context
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	FireCollision->OnComponentBeginOverlap.AddDynamic(this, &AEggPlayer::OnFireOverlap);

}

void AEggPlayer::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	CanJump = true;
}

// Called to bind functionality to input
void AEggPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);



	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {

		// ControlBallとIA_ControlのTriggeredをBindする
		EnhancedInputComponent->BindAction(ControlAction, ETriggerEvent::Triggered, this, &AEggPlayer::ControlBall);

		// LookとIA_LookのTriggeredをBindする
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEggPlayer::Look);

		// JumpとIA_JumpのTriggeredをBindする
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AEggPlayer::Jump);

		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Triggered, this, &AEggPlayer::Boost);
	}
}

void AEggPlayer::OnGoalReached()
{
	if (bIsGoalReached) return; // 二重判定防止
	bIsGoalReached = true;

	// 動きを止める
	Sphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Sphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Sphere->SetSimulatePhysics(false); // ← 完全停止！

	// “CLEAR”テキストを表示
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeUIOnly());

		// シンプルにHUD表示
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("CLEAR!"));
	}
}



void AEggPlayer::ControlBall(const FInputActionValue& Value)
{
	// inputのValueはVector2Dに変換できる
	FVector2D V = Value.Get<FVector2D>();

	// Vectorを計算する
	FVector ForceVector = FVector(V.Y, V.X, 0.0f) * Speed;

	// Sphereに力を加える
	Sphere->AddForce(ForceVector, TEXT("NONE"), true);
}


void AEggPlayer::Look(const FInputActionValue& Value)
{
	// inputのValueはVector2Dに変換できる
	const FVector2D V = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(V.X);
		AddControllerPitchInput(-V.Y);

		// Pawnが持っているControlの角度を取得する
		FRotator ControlRotate = GetControlRotation();

		// controllerのPitchの角度を制限する
		//double LimitPitchAngle = FMath::ClampAngle(ControlRotate.Pitch, -40.0f, -10.0f);

		// PlayerControllerの角度を設定する
		//UGameplayStatics::GetPlayerController(this, 0)->SetControlRotation(FRotator(LimitPitchAngle, ControlRotate.Yaw, 0.0f));
	}
}

void AEggPlayer::Jump(const FInputActionValue& Value)
{
	// inputのValueはboolに変換できる
	if (const bool V = Value.Get<bool>() && CanJump)
	{
		Sphere->AddImpulse(FVector(0.0f, 0.0f, JumpImpulse), TEXT("None"), true);
		CanJump = false;
	}
}

void AEggPlayer::OnFireOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		OtherActor->Destroy();
	}
}

void AEggPlayer::Boost(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Boost Triggered!"));

	if (!BoostEffect || !Sphere) return;

	UNiagaraFunctionLibrary::SpawnSystemAttached(
		BoostEffect,
		Sphere,
		NAME_None,
		FVector(0.0f, 0.0f, 200.0f),
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true
	);

	// 炎の判定を一時的に有効化
	FireCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 1秒後に無効化
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			FireCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}, 1.0f, false);
}

