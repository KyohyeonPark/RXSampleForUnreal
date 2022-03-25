// Copyright Epic Games, Inc. All Rights Reserved.

#include "RxSamplePlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "RxSampleCharacter.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

rxcpp::schedulers::run_loop RunLoop;

ARxSamplePlayerController::ARxSamplePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ARxSamplePlayerController::BeginPlay()
{
	Super::BeginPlay();

	GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	SubscribeMoveLog();
	SubscribeMoveCommand();
	SubscribeCameraCommand();
}

void ARxSamplePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//Moving
	//Clicked;
	//Tick;
}

void ARxSamplePlayerController::SubscribeMoveLog()
{
	auto MovingStream = Moving.get_observable();

	MovingStream
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

	MovingStream
		.filter([](bool b) { return b == true; })
		.subscribe(
			[](bool b)
			{
				const FString Msg("Moving...");
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Green, Msg);
			}
	);

	MovingStream
		.filter([](bool b) { return b == false; })
		.subscribe(
			[](bool b)
			{
				const FString Msg("Stop");
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::White, Msg);
			}
	);
}

void ARxSamplePlayerController::SubscribeMoveCommand()
{
	// �ٸ� Rx ����ü�ʹ� �޸� RxCpp���� Throttle, Buffer_Toggle(Buffer�� closingSelector ����)�� �����Ǿ� ���� �ʴ�.
	// ���� ��� throttle �� �ܺ� �ڵ� �ݿ�, buffer_toggle�� window_toggle �� ��ü�Ͽ� ��ȸ�� �����Ͽ���.
	// ������� ������ ������ �ƴ����� ���� ������ �ʿ��� operator ���� �� ���� �������� �ϴ� ���ܵд�.
	auto DoubleClickPeriod = std::chrono::milliseconds(200);

	auto MainThread = rxcpp::observe_on_run_loop(RunLoop);
	//auto WorkThread = rxcpp::synchronize_new_thread();

	auto ClickStream = Tick.get_observable()
		.map([this](float DeltaTime) { return bMoveToMouseCursor; });

	auto FlushStream = ClickStream
		.filter([](uint32 bMove) { return bMove == 1; })
		.throttle(DoubleClickPeriod);

	auto WindowStream = ClickStream
		.distinct_until_changed()
		.window_toggle(
			FlushStream,
			[=](uint32 bMove) {
				return FlushStream
					.delay(DoubleClickPeriod);
			}
	);

	WindowStream
		.observe_on(MainThread)
		.subscribe([MainThread, this](rxcpp::observable<uint32> ClickWindow)
		{
			MoveToMouseCursor();

			ClickWindow
				.filter([](uint32 bMove) { return bMove == 1; })
				.take(2)
				.observe_on(MainThread)
				.subscribe([this](uint32 bMove)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::Printf(TEXT("Double clicked!")));

						GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
					});
		});
}

void ARxSamplePlayerController::SubscribeCameraCommand()
{
	/*
	auto TriggerStart = Clicked.get_observable()
		.filter([](bool bDown) { return bDown == true; });
	auto TriggerEnd = Clicked.get_observable()
		.filter([](bool bDown) { return bDown == false; });
	auto CameraMoveStream = Tick.get_observable()
		.skip_until(TriggerStart)
		.map([this](float DeltaTime) { return bMoveToMouseCursor; })
		.take_until(TriggerEnd)
		.repeat();
	CameraMoveStream
		.subscribe([]() 
			{
				//AddControllerYawInput();
				//AddPitchInput(Val);
			});
	*/
}

void ARxSamplePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

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

	/*
	while (lifetime.is_subscribed() || !RunLoop.empty()) {
		while (!RunLoop.empty() && RunLoop.peek().when < RunLoop.now()) {
			RunLoop.dispatch();
		}
	}*/
	if (!RunLoop.empty() && RunLoop.peek().when < RunLoop.now())
		RunLoop.dispatch();
}

void ARxSamplePlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &ARxSamplePlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &ARxSamplePlayerController::OnSetDestinationReleased);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ARxSamplePlayerController::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ARxSamplePlayerController::StopJumping);

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

void ARxSamplePlayerController::Jump()
{
	GetCharacter()->Jump();
}

void ARxSamplePlayerController::StopJumping()
{
	GetCharacter()->StopJumping();
}
