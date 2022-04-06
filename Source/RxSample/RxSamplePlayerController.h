#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "Rx.h"
#include "RxSamplePlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UNiagaraSystem;

UCLASS()
class ARxSamplePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARxSamplePlayerController();

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float ShortPressThreshold;

	/** FX Class that we will spawn when clicking */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UNiagaraSystem* FXCursor;

protected:
	bool bMoving = false;
	rxcpp::subjects::subject<bool> Moving;
	rxcpp::subjects::subject<bool> Clicked;
	rxcpp::subjects::subject<float> Tick;

	const float WalkSpeed = 200.f;
	const float RunSpeed = 600.f;
	const float ZoomUnit = 100.f;
	const float ZoomMin = 100.f;
	const float ZoomMax = 3000.f;

	// Begin PlayerController interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();
	void OnTouchPressed(const ETouchIndex::Type FingerIndex, const FVector Location);
	void OnTouchReleased(const ETouchIndex::Type FingerIndex, const FVector Location);
	void OnCameraMovePressed();
	void OnCameraMoveReleased();
	void Jump();
	void StopJumping();
	void ZoomIn();
	void ZoomOut();

	void SubscribeMoveLog();
	void SubscribeMoveCommand();
	void SubscribeCameraCommand();

private:
	bool bInputPressed; // Input is bring pressed
	bool bIsTouch; // Is it a touch device
	bool bCameraMove;
	float FollowTime; // For how long it has been pressed
};


