// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Rx.h"
#include "RxSamplePlayerController.generated.h"

UCLASS()
class ARxSamplePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARxSamplePlayerController();

protected:
	bool bMoving = false;
	rxcpp::subjects::subject<bool> Moving;
	rxcpp::subjects::subject<bool> Clicked;
	rxcpp::subjects::subject<float> Tick;

	const float WalkSpeed = 200.f;
	const float RunSpeed = 600.f;
	const float ZoomUnit = 100.f;
	const float ZoomMin = 100.f;
	const float ZoomMax = 1500.f;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	uint32 bCameraMove : 1;

	// Begin PlayerController interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	// End PlayerController interface

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	void MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location);
	
	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();
	void OnCameraMovePressed();
	void OnCameraMoveReleased();
	void Jump();
	void StopJumping();
	void ZoomIn();
	void ZoomOut();

	void SubscribeMoveLog();
	void SubscribeMoveCommand();
	void SubscribeCameraCommand();
};


