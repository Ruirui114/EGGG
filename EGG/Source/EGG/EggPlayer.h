#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "EggPlayer.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UArrowComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class EGG_API AEggPlayer : public APawn
{
	GENERATED_BODY()

public:
	AEggPlayer();

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** BallをControlする */
	void ControlBall(const FInputActionValue& Value);

	/** 視点を操作する */
	void Look(const FInputActionValue& Value);

	/** ジャンプする */
	void Jump(const FInputActionValue& Value);

public:
	void OnGoalReached();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;



private:
	/** キャラクターの球体メッシュ */
	UPROPERTY(VisibleAnywhere, Category = Character)
	UStaticMeshComponent* Sphere;

	/** カメラ関係 */
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* Camera;

	/** 入力関連 */
	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ControlAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* BoostAction;

	/** 炎の発生位置を固定するためのノード */
	UPROPERTY(VisibleAnywhere, Category = Effect)
	USceneComponent* FireSpawnPoint;

	/** 炎の当たり判定（Sphere） */
	UPROPERTY(VisibleAnywhere, Category = Effect)
	USphereComponent* FireCollision;

	/** 炎エフェクト（Niagara） */
	UPROPERTY(EditAnywhere, Category = Effect)
	UNiagaraSystem* BoostEffect;

	/** 実際に出しているエフェクトの参照 */
	UPROPERTY()
	UNiagaraComponent* ActiveBoostEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float GroundCheckDistance = 50.0f; // 足元からのチェック距離

	// 地面に触れているかどうか
	bool bIsGrounded = false;

	/** 炎が出てる間は true */
	bool bIsBoosting = false;

	/** 3秒後に消すためのタイマー */
	FTimerHandle BoostTimerHandle;

	/** 炎が当たったときの処理 */
	UFUNCTION()
	void OnFireOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** 炎を出す関数 */
	void Boost(const FInputActionValue& Value);

	/** 炎を止める関数 */
	void StopBoost();

	/** ゴール関連 */
	bool bIsGoalReached = false;

	/** 各種設定値 */
	float Speed = 300.0f;
	float Health = 100.0f;
	float JumpImpulse = 1000.0f;
	/** 空中での操作の強さ（0.0〜1.0） */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float AirControlFactor = 0.7f;
	bool CanJump = false;
};
