// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "BlueBoatController.h"

ABlueBoatController::ABlueBoatController(const FObjectInitializer& ObjectInitializer)
	: AHolodeckPawnController(ObjectInitializer) {
	UE_LOG(LogTemp, Warning, TEXT("BlueBoat Controller Initialized"));
}

ABlueBoatController::~ABlueBoatController() {}
