// Copyright Epic Games, Inc. All Rights Reserved.

#include "RxSampleGameMode.h"
#include "RxSamplePlayerController.h"
#include "RxSampleCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARxSampleGameMode::ARxSampleGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = ARxSamplePlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}