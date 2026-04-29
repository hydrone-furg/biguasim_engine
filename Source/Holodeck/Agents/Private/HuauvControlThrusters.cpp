#include "Holodeck.h"
#include "HuauvControlThrusters.h"

UHuauvControlThrusters::UHuauvControlThrusters(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer) {}

void UHuauvControlThrusters::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (Huauv == nullptr) {
		Huauv = static_cast<AHuauv*>(HuauvController->GetPawn());
		if (Huauv == nullptr) {

			UE_LOG(LogHolodeck, Error, TEXT("UHuauvControlThrusters couldn't get Huauv reference"));
			return;
		}
		
		Huauv->EnableDamping();
	}

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	// Huauv->ApplyBuoyantForce();

	//Criar logica para troca de domínio
	float CurrentPositionZ = Huauv->GetActorLocation().Z;
	if(CurrentPositionZ <= 0){
		Huauv->ApplyBuoyantForce();
		Huauv->ApplyUnderwaterThrusters(InputCommandFloat);
	}else{
		Huauv->ApplyAirThrusters(InputCommandFloat);
	}
	
	// Zero out the physics based controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}
