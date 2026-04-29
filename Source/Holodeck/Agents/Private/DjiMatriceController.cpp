
#include "Holodeck.h"
#include "DjiMatriceController.h"

ADjiMatriceController::ADjiMatriceController(const FObjectInitializer& ObjectInitializer)
	: AHolodeckPawnController(ObjectInitializer) {
	UE_LOG(LogTemp, Warning, TEXT("DjiMatrice Controller Initialized"));
}

ADjiMatriceController::~ADjiMatriceController() {}

