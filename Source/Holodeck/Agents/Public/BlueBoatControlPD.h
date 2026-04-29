#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "BlueBoat.h"
#include "HolodeckControlScheme.h"
#include "SimplePID.h"
#include <math.h>

#include "BlueBoatControlPD.generated.h"

const float BB_CONTROL_MAX_FORCE = 1000;
const float BB_CONTROL_MAX_TORQUE = 1000;

const float BB_POS_P = 100;
const float BB_POS_D = 50;

const float BB_YAW_P = 15;
const float BB_YAW_D = 7;

/**
* UBlueBoatControlPD
*/
UCLASS()
class HOLODECK_API UBlueBoatControlPD : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	UBlueBoatControlPD(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	unsigned int GetControlSchemeSizeInBytes() const override {
		return 2 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { BlueBoatController = Controller; };

private:
	AHolodeckPawnController* BlueBoatController;
	ABlueBoat* BlueBoat;

	SimplePID PositionController;
	SimplePID YawController;

	// How far from center thrusters are
	float d = 0;
};
