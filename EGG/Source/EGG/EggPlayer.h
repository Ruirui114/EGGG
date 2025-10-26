// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "NiagaraSystem.h" 

#include "Components/SphereComponent.h"
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
	// Sets default values for this pawn's properties
	AEggPlayer();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Hit EventをBindingする関数
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** BallをControlする */
	void ControlBall(const FInputActionValue& Value);

	/** 視点を操作する */
	void Look(const FInputActionValue& Value);

	// ジャンプする
	void Jump(const FInputActionValue& Value);
	
public:
	void OnGoalReached(); // ゴール時に呼ばれる関数

public:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	/** Character用のStaticMesh : Sphere */
	UPROPERTY(VisibleAnywhere, Category = Character, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Sphere;

	/** Cameraを配置するためのSpringArm */
	UPROPERTY(VisibleAnywhere, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	/** SpringArmの先端に配置するカメラ */
	UPROPERTY(VisibleAnywhere, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> Camera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Control Input Action */
	UPROPERTY(EditAnywhere, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ControlAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	/** シフト入力（炎を出す） */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<UInputAction> BoostAction;

	/** 炎のNiagaraエフェクト */
	UPROPERTY(EditAnywhere, Category = Effect)
	UNiagaraSystem* BoostEffect;

	/** 炎の当たり判定用 Sphere */
	UPROPERTY(VisibleAnywhere, Category = Effect)
	USphereComponent* FireCollision;

	/** 炎に当たったオブジェクトを消す関数 */
	UFUNCTION()
	void OnFireOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** 炎を出す関数 */
	void Boost(const FInputActionValue& Value);

	bool bIsGoalReached = false;

	// 速度
	float Speed = 200.0f;

	// 体力
	float Health = 100.0f;

	// ジャンプに加える力
	float JumpImpulse = 1000.0f;

	// ジャンプができるか判定するフラグ
	bool CanJump = false;
};
