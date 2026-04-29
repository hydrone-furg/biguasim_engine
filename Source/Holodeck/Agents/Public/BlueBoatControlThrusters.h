#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "BlueBoat.h"
#include "HolodeckControlScheme.h"
#include "SimplePID.h"
#include <math.h>

#include "BlueBoatControlThrusters.generated.h"

/**
* UBlueBoatControlThrusters
*/
UCLASS()
class HOLODECK_API UBlueBoatControlThrusters : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	UBlueBoatControlThrusters(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	unsigned int GetControlSchemeSizeInBytes() const override {
		return 2 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { BlueBoatController = Controller; };

private:
	AHolodeckPawnController* BlueBoatController;
	ABlueBoat* BlueBoat;

};
