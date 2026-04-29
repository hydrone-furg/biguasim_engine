// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "BlueBoatControlThrusters.h"
#include "BlueBoatControlPD.h"
#include "BlueBoat.h"

#include "BlueBoatController.generated.h"

/**
* A Holodeck Turtle Agent Controller
*/
UCLASS()
class HOLODECK_API ABlueBoatController : public AHolodeckPawnController
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	ABlueBoatController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Default Destructor
	*/
	~ABlueBoatController();

	void AddControlSchemes() override {
		// The default controller currently in ControlSchemes index 0 is the dynamics one. We push it back to index 2 with this code.

		// Thruster controller
		UBlueBoatControlThrusters* Thrusters = NewObject<UBlueBoatControlThrusters>();
		Thrusters->SetController(this);
		ControlSchemes.Insert(Thrusters, 0);

		// Position / orientation controller
		UBlueBoatControlPD* PD = NewObject<UBlueBoatControlPD>();
		PD->SetController(this);
		ControlSchemes.Insert(PD, 1);
	}
};
