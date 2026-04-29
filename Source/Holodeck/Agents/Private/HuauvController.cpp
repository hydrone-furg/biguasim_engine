
#include "Holodeck.h"
#include "HuauvController.h"

AHuauvController::AHuauvController(const FObjectInitializer& ObjectInitializer)
	: AHolodeckPawnController(ObjectInitializer) {
	UE_LOG(LogTemp, Warning, TEXT("Huauv Controller Initialized"));
}

AHuauvController::~AHuauvController() {}

