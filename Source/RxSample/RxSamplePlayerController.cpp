#include "RxSamplePlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "RxSampleCharacter.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


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

	AddPitchInput(15.f);

	SubscribeMoveLog();
	SubscribeMoveCommand();
	SubscribeCameraCommand();
}

void ARxSamplePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, Msg);
			},
			[]()
			{
				const FString Msg("Unsubscribed observable for first move.");
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, Msg);
			}
			);

	MovingStream
		.filter([](bool b) { return b == true; })
		.subscribe(
			[](bool b)
			{
				const FString Msg("Moving...");
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, Msg);
			}
	);

	MovingStream
		.filter([](bool b) { return b == false; })
		.subscribe(
			[](bool b)
			{
				const FString Msg("Stop");
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, Msg);
			}
	);
}

void ARxSamplePlayerController::SubscribeMoveCommand()
{
	// 다른 Rx 구현체와는 달리 RxCpp에는 Throttle, Buffer_Toggle(Buffer의 closingSelector 버전)이 구현되어 있지 않다.
	// 따라서 대신 throttle 은 외부 코드 반영, buffer_toggle은 window_toggle 로 대체하여 우회로 구현하였다.
	// 여러모로 적절한 구현은 아니지만 향후 구현이 필요한 operator 참고 및 예제 차원에서 일단 남겨둔다.
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
	auto Stream = Tick.get_observable()
		.map([this](float DeltaTime) { return bCameraMove; })
		.distinct_until_changed();

	auto TriggerStart = Stream
		.filter([](uint32 bDown) { return bDown == 1; });
	auto TriggerEnd = Stream
		.filter([](uint32 bDown) { return bDown == 0; });
	auto CameraMoveStream = Tick.get_observable()
		.skip_until(TriggerStart)
		.map([this](float DeltaTime)
			{
				FVector2D Pos;
				GetInputMouseDelta(Pos.X, Pos.Y);
				return MoveTemp(Pos);
			}
		)
		.take_until(TriggerEnd)
		.repeat();
	CameraMoveStream
		.subscribe([this](const FVector2D& V)
			{
				if (ARxSampleCharacter* MyPawn = Cast<ARxSampleCharacter>(GetPawn()))
				{
					FRotator Rotation(V.Y, V.X, 0.f);
					MyPawn->GetCameraBoom()->AddRelativeRotation(Rotation);
				}
			});
}

void ARxSamplePlayerController::PlayerTick(float DeltaTime)
{
	// GameInstance 혹은 모듈 레벨에서 틱을 다룰 수 있는 쪽으로 옮긴다.
	if (!RunLoop.empty() && RunLoop.peek().when < RunLoop.now())
		RunLoop.dispatch();

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
}

void ARxSamplePlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &ARxSamplePlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &ARxSamplePlayerController::OnSetDestinationReleased);
	InputComponent->BindAction("MoveCamera", IE_Pressed, this, &ARxSamplePlayerController::OnCameraMovePressed);
	InputComponent->BindAction("MoveCamera", IE_Released, this, &ARxSamplePlayerController::OnCameraMoveReleased);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ARxSamplePlayerController::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ARxSamplePlayerController::StopJumping);
	InputComponent->BindAction("ZoomIn", IE_Pressed, this, &ARxSamplePlayerController::ZoomIn);
	InputComponent->BindAction("ZoomOut", IE_Pressed, this, &ARxSamplePlayerController::ZoomOut);

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

void ARxSamplePlayerController::OnCameraMovePressed()
{
	bCameraMove = true;
}

void ARxSamplePlayerController::OnCameraMoveReleased()
{
	bCameraMove = false;
}

void ARxSamplePlayerController::Jump()
{
	GetCharacter()->Jump();
}

void ARxSamplePlayerController::StopJumping()
{
	GetCharacter()->StopJumping();
}

void ARxSamplePlayerController::ZoomIn()
{
	if (ARxSampleCharacter* MyPawn = Cast<ARxSampleCharacter>(GetPawn()))
	{
		float& TargetArmLength = MyPawn->GetCameraBoom()->TargetArmLength;
		TargetArmLength = FMath::Clamp(TargetArmLength - ZoomUnit, ZoomMin, ZoomMax);
	}
}

void ARxSamplePlayerController::ZoomOut()
{
	if (ARxSampleCharacter* MyPawn = Cast<ARxSampleCharacter>(GetPawn()))
	{
		float& TargetArmLength = MyPawn->GetCameraBoom()->TargetArmLength;
		TargetArmLength = FMath::Clamp(TargetArmLength + ZoomUnit, ZoomMin, ZoomMax);
	}
}
