// Copyright Epic Games, Inc. All Rights Reserved.

#include "RxSamplePlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "RxSampleCharacter.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ARxSamplePlayerController::ARxSamplePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ARxSamplePlayerController::BeginPlay()
{
	Super::BeginPlay();

	GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	SubscribeMovingLog();
	SubscribeRunCommand();
}

void ARxSamplePlayerController::SubscribeMovingLog()
{
	Moving.get_observable()
		.filter([](bool b) { return b == true; })
		.first()
		.subscribe(
			[](bool b)
			{
				const FString Msg("First Move!");
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Blue, Msg);
			},
			[]()
			{
				const FString Msg("Unsubscribed observable for first move.");
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::White, Msg);
			}
			);

	Moving.get_observable()
		.filter([](bool b) { return b == true; })
		.subscribe(
			[](bool b)
			{
				const FString Msg("Moving...");
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, Msg);
			}
	);

	Moving.get_observable()
		.filter([](bool b) { return b == false; })
		.subscribe(
			[](bool b)
			{
				const FString Msg("Stop");
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::White, Msg);
			}
	);
}

void ARxSamplePlayerController::SubscribeRunCommand()
{
	// 다른 Rx 구현제와는 달리 RxCpp에는 Buffer_Toggle(Buffer의 closingSelector 버전)이 구현되어 있지 않다.
	// 따라서 대신 window_toggle 을 이용하여 우회로 구현하였다.
	// 여러모로 적절한 구현은 아니지만 예제 차원에서 일단 남겨둔다.
	auto period = std::chrono::milliseconds(200);

	auto ClickStream = Clicked.get_observable()
		.filter([this](bool b) { return b == true; });
	auto FlushStream = ClickStream.debounce(period, rxcpp::observe_on_event_loop());
	auto WindowStream = ClickStream
		.window_toggle(
			FlushStream.start_with(false),
			[=](bool b) {
				return FlushStream;
			}
	);
	auto CountStream = WindowStream
		.map([](rxcpp::observable<bool> w) {
				return w
					.count()
					.as_dynamic();
			}
		)
		.merge()
		.filter([](int count) { return count >= 2; });

	CountStream
		.subscribe(
			[this](int ClickCount)
			{
				const FString Msg("Double clicked!");
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::Printf(TEXT("Double clicked! ClickCount: %d"), ClickCount));

				GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
			}
		);

	//FString str(strlen(typeid(n).name()), typeid(n).name());
	//UE_LOG(LogTemp, Log, TEXT("TYPE: %s"), *str);
}

void ARxSamplePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}

	if (UPathFollowingComponent* PathFollowingComp = FindComponentByClass<UPathFollowingComponent>())
	{
		bool bAlreadyAtGoal = PathFollowingComp->DidMoveReachGoal();
		if (bMoving != !bAlreadyAtGoal)
		{
			bMoving = !bAlreadyAtGoal;
			Moving.get_subscriber().on_next(bMoving);
			GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		}
	}

	Tick.get_subscriber().on_next(DeltaTime);
}

void ARxSamplePlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &ARxSamplePlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &ARxSamplePlayerController::OnSetDestinationReleased);

	// support touch devices 
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ARxSamplePlayerController::MoveToTouchLocation);
	InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &ARxSamplePlayerController::MoveToTouchLocation);

	InputComponent->BindAction("ResetVR", IE_Pressed, this, &ARxSamplePlayerController::OnResetVR);
}

void ARxSamplePlayerController::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ARxSamplePlayerController::MoveToMouseCursor()
{
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		if (ARxSampleCharacter* MyPawn = Cast<ARxSampleCharacter>(GetPawn()))
		{
			if (MyPawn->GetCursorToWorld())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, MyPawn->GetCursorToWorld()->GetComponentLocation());
			}
		}
	}
	else
	{
		// Trace to see what is under the mouse cursor
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit)
		{
			// We hit something, move there
			SetNewMoveDestination(Hit.ImpactPoint);
		}
	}
}

void ARxSamplePlayerController::MoveToTouchLocation(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	if (HitResult.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(HitResult.ImpactPoint);
	}
}

void ARxSamplePlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
	{
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if ((Distance > 120.0f))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
		}
	}
}

void ARxSamplePlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
	Clicked.get_subscriber().on_next(true);
}

void ARxSamplePlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
	Clicked.get_subscriber().on_next(false);
}
