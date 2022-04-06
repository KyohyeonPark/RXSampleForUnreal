#include "RxSamplePlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "RxSampleCharacter.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

rxcpp::schedulers::run_loop RunLoop;


ARxSamplePlayerController::ARxSamplePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
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

void ARxSamplePlayerController::PlayerTick(float DeltaTime)
{
	// GameInstance 혹은 모듈 레벨에서 틱을 다룰 수 있는 쪽으로 옮긴다.
	if (!RunLoop.empty() && RunLoop.peek().when < RunLoop.now())
		RunLoop.dispatch();

	Super::PlayerTick(DeltaTime);
/*
	if(bInputPressed)
	{
		FollowTime += DeltaTime;

		// Look for the touch location
		FVector HitLocation = FVector::ZeroVector;
		FHitResult Hit;
		if(bIsTouch)
		{
			GetHitResultUnderFinger(ETouchIndex::Touch1, ECC_Visibility, true, Hit);
		}
		else
		{
			GetHitResultUnderCursor(ECC_Visibility, true, Hit);
		}
		HitLocation = Hit.Location;

		// Direct the Pawn towards that location
		APawn* const MyPawn = GetPawn();
		if(MyPawn)
		{
			FVector WorldDirection = (HitLocation - MyPawn->GetActorLocation()).GetSafeNormal();
			MyPawn->AddMovementInput(WorldDirection, 1.f, false);
		}
	}
	else
	{
		FollowTime = 0.f;
	}
*/
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
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ARxSamplePlayerController::OnTouchPressed);
	InputComponent->BindTouch(EInputEvent::IE_Released, this, &ARxSamplePlayerController::OnTouchReleased);

}

void ARxSamplePlayerController::MoveToMouseCursor()
{
	// If it was a short press
//	if (FollowTime <= ShortPressThreshold)
	{
		// We look for the location in the world where the player has pressed the input
		FVector HitLocation = FVector::ZeroVector;
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, true, Hit);
		HitLocation = Hit.Location;

		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitLocation);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, HitLocation, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}
}

void ARxSamplePlayerController::OnSetDestinationPressed()
{
	// We flag that the input is being pressed
	bInputPressed = true;
	// Just in case the character was moving because of a previous short press we stop it
	StopMovement();

	Clicked.get_subscriber().on_next(true);
}

void ARxSamplePlayerController::OnSetDestinationReleased()
{
	// Player is no longer pressing the input
	bInputPressed = false;

	// If it was a short press
	if(FollowTime <= ShortPressThreshold)
	{
		// We look for the location in the world where the player has pressed the input
		FVector HitLocation = FVector::ZeroVector;
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, true, Hit);
		HitLocation = Hit.Location;

		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitLocation);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, HitLocation, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	Clicked.get_subscriber().on_next(false);
}

void ARxSamplePlayerController::OnTouchPressed(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	bIsTouch = true;
	OnSetDestinationPressed();
}

void ARxSamplePlayerController::OnTouchReleased(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	bIsTouch = false;
	OnSetDestinationReleased();
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
		.map([this](float DeltaTime) { return bInputPressed; });

	auto FlushStream = ClickStream
		.filter([](bool bPressed) { return bPressed == true; })
		.throttle(DoubleClickPeriod);

	auto WindowStream = ClickStream
		.distinct_until_changed()
		.window_toggle(
			FlushStream,
			[=](bool bPressed) {
				return FlushStream
					.delay(DoubleClickPeriod);
			}
	);

	WindowStream
		.observe_on(MainThread)
		.subscribe([MainThread, this](rxcpp::observable<bool> ClickWindow)
			{
				MoveToMouseCursor();

				ClickWindow
					.filter([](bool bPressed) { return bPressed == true; })
					.take(2)
					.observe_on(MainThread)
					.subscribe([this](bool bPressed)
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
