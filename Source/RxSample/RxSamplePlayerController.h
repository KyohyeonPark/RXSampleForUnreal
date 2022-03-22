// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#pragma push_macro("check")
#undef check
#pragma push_macro("ensure")
#undef ensure

#include "rxcpp/rx.hpp"

#pragma pop_macro("check")
#pragma pop_macro("ensure")

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

	float WalkSpeed = 200.f;
	float RunSpeed = 600.f;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	// Begin PlayerController interface
	virtual void BeginPlay() override;
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

	void SubscribeMovingLog();
	void SubscribeRunCommand();
};


