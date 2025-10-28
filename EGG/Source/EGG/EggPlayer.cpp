#include "EggPlayer.h"

#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
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
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
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

	
	UPhysicalMaterial* PhysMat = NewObject<UPhysicalMaterial>();
	PhysMat->Restitution = 0.0f;  // バウンド無効
	PhysMat->Friction = 0.8f;     // 必要に応じて摩擦
	Sphere->SetPhysMaterialOverride(PhysMat);

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

	// 炎の位置固定用コンポーネント（Rootではなくワールドに追従）
	FireSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FireSpawnPoint"));
	FireSpawnPoint->SetupAttachment(RootComponent);

	// FireCollision を FireSpawnPoint にアタッチ
	FireCollision = CreateDefaultSubobject<USphereComponent>(TEXT("FireCollision"));
	FireCollision->SetupAttachment(FireSpawnPoint);
	FireCollision->InitSphereRadius(100.f);
	FireCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

	if (FireCollision)
	{
		FireCollision->OnComponentBeginOverlap.AddDynamic(this, &AEggPlayer::OnFireOverlap);
	}

}

// Tick関数で位置だけを同期（回転は無視）
void AEggPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 接地判定
	FVector Start = Sphere->GetComponentLocation();
	FVector End = Start - FVector(0, 0, GroundCheckDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (ActiveBoostEffect)
	{
		FVector BallLocation = Sphere->GetComponentLocation();
		FVector FireOffset = FVector(0, 0, 47); // ボールの下に出す例
		ActiveBoostEffect->SetWorldLocation(BallLocation + FireOffset);
	}

	if (bHit)
	{
		bIsGrounded = true;
		// バウンド防止
		FVector Vel = Sphere->GetPhysicsLinearVelocity();
		if (Vel.Z < 0) Vel.Z = 0;
		Sphere->SetPhysicsLinearVelocity(Vel);
	}
	else
	{
		bIsGrounded = false;
	}
}

void AEggPlayer::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	
	// 接地判定：Z方向がほぼ上向きの面に接触した場合
	if (HitNormal.Z > 0.7f)
	{
		bIsGrounded = true;   // 地面に着地した
		CanJump = true;       // ジャンプ可能

		// バウンド防止
		FVector Vel = Sphere->GetPhysicsLinearVelocity();
		Vel.Z = 0.0f;
		Sphere->SetPhysicsLinearVelocity(Vel);
	}
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

		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Started, this, &AEggPlayer::Boost);


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
	FVector2D MoveValue = Value.Get<FVector2D>();
	if (!Controller || MoveValue.IsNearlyZero()) return;

	// カメラ方向に合わせた移動方向を計算
	FRotator CameraRot = Camera->GetComponentRotation();
	FVector Forward = FRotationMatrix(CameraRot).GetScaledAxis(EAxis::X);
	FVector Right = FRotationMatrix(CameraRot).GetScaledAxis(EAxis::Y);

	Forward.Z = 0.0f;
	Right.Z = 0.0f;
	Forward.Normalize();
	Right.Normalize();

	FVector MoveDir = (Forward * MoveValue.Y + Right * MoveValue.X).GetSafeNormal();

	FVector CurrentVel = Sphere->GetPhysicsLinearVelocity();
	FVector FlatVel = FVector(CurrentVel.X, CurrentVel.Y, 0.0f);

	// --- 逆方向入力時の減速処理 ---
	if (!FlatVel.IsNearlyZero())
	{
		float Dot = FVector::DotProduct(FlatVel.GetSafeNormal(), MoveDir);

		if (Dot < -0.5f) // ←真逆に近い方向を押したら
		{
			// 💨 徐々に減速（0.85で減速率を調整）
			FVector NewVel = FlatVel * 0.85f;

			// 少しブレーキをかけるが完全には止めない
			Sphere->SetPhysicsLinearVelocity(FVector(NewVel.X, NewVel.Y, CurrentVel.Z));

			// ほんの少しだけ逆方向に力を加えて反転を始める
			float ControlStrength = bIsGrounded ? 0.5f : AirControlFactor * 0.5f;
			Sphere->AddForce(MoveDir * Speed * Sphere->GetMass() * ControlStrength);

			return; // このフレームではこれで終わり
		}
	}

	// --- 通常の移動処理 ---
	if (FlatVel.Size() < 2000.0f)
	{
		float ControlStrength = bIsGrounded ? 1.0f : AirControlFactor;
		Sphere->AddForce(MoveDir * Speed * Sphere->GetMass() * ControlStrength);
	}
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
	bool bPressed = Value.Get<bool>();

	if (bPressed && bIsGrounded)
	{
		// Z方向の速度をリセット
		FVector Vel = Sphere->GetPhysicsLinearVelocity();
		Vel.Z = 0.0f;
		Sphere->SetPhysicsLinearVelocity(Vel);

		// ジャンプ
		Sphere->AddImpulse(FVector(0.0f, 0.0f, JumpImpulse), NAME_None, true);

		// 空中フラグ
		bIsGrounded = false;
	}
}

void AEggPlayer::OnFireOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor->ActorHasTag("Destructible"))
	{
		OtherActor->Destroy();
	}
}

void AEggPlayer::Boost(const FInputActionValue& Value)
{
	if (bIsBoosting || !BoostEffect) return;

	bIsBoosting = true;

	// 🔥 SpawnSystemAttached の戻り値を保存！
	//ActiveBoostEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
	//	BoostEffect,
	//	Sphere,
	//	NAME_None,
	//	FVector(0.0f,0.0f,47.0f),
	//	FRotator::ZeroRotator,
	//	EAttachLocation::KeepRelativeOffset,
	//	true
	//);

	if (ActiveBoostEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔥 Boost Start!"));
	}

	// 🔥 当たり判定を有効化
	FireCollision->SetSphereRadius(300.f); // 当たり判定を広げる
	FireCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 3秒後に炎を止める
	GetWorld()->GetTimerManager().SetTimer(
		BoostTimerHandle,
		this,
		&AEggPlayer::StopBoost,
		3.0f,
		false
	);

	if (BoostEffect)
	{
		ActiveBoostEffect = NewObject<UNiagaraComponent>(this, UNiagaraComponent::StaticClass());
		ActiveBoostEffect->SetAsset(BoostEffect);
		ActiveBoostEffect->AttachToComponent(FireSpawnPoint, FAttachmentTransformRules::KeepRelativeTransform);
		ActiveBoostEffect->SetRelativeLocation(FVector(50, 0, 50)); // 位置
		ActiveBoostEffect->SetWorldScale3D(FVector(10.0f));        // ← サイズ変更
		ActiveBoostEffect->RegisterComponent();
		ActiveBoostEffect->Activate();
	}
}

void AEggPlayer::StopBoost()
{
	if (!bIsBoosting) return;
	bIsBoosting = false;

	UE_LOG(LogTemp, Warning, TEXT("🔥 Boost stopped!"));

	if (ActiveBoostEffect)
	{
		ActiveBoostEffect->Deactivate();
		ActiveBoostEffect->DestroyComponent(); // コンポーネントを削除して安全に消す
		ActiveBoostEffect = nullptr;
	}
}

