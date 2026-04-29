#include "Holodeck.h"
#include "DjiMatriceControlThrusters.h"

UDjiMatriceControlThrusters::UDjiMatriceControlThrusters(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer) {}

void UDjiMatriceControlThrusters::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (DjiMatrice == nullptr) {
		DjiMatrice = static_cast<ADjiMatrice*>(DjiMatriceController->GetPawn());
		if (DjiMatrice == nullptr) {

			UE_LOG(LogHolodeck, Error, TEXT("UDjiMatriceControlThrusters couldn't get DjiMatrice reference"));
			return;
		}
		DjiMatrice->ForcesInMainController(false);
	}

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	DjiMatrice->ApplyFlightForce();
	DjiMatrice->ApplyThrusters(InputCommandFloat);
	
	// Zero out the physics based controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}
