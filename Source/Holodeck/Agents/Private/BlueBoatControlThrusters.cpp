#include "Holodeck.h"
#include "BlueBoatControlThrusters.h"


UBlueBoatControlThrusters::UBlueBoatControlThrusters(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer) {}

void UBlueBoatControlThrusters::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (BlueBoat == nullptr) {
		BlueBoat = static_cast<ABlueBoat*>(BlueBoatController->GetPawn());
		if (BlueBoat == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("UBlueBoatControlThrusters couldn't get BlueBoat reference"));
			return;
		}
		
		BlueBoat->EnableDamping();
	}

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	BlueBoat->ApplyBuoyantForce();
	BlueBoat->ApplyThrusters(InputCommandFloat);

	// Zero out the physics based controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}