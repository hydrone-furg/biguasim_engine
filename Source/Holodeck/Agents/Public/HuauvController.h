// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "HuauvControlThrusters.h"
// #include "HuauvControlPD.h"
#include "Huauv.h"
//#include "BlueROV2.generated.h"


#include "HuauvController.generated.h"

/**
* A Holodeck Turtle Agent Controller
*/
UCLASS()

class HOLODECK_API AHuauvController : public AHolodeckPawnController
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	AHuauvController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Default Destructor
	*/
	~AHuauvController();

	void AddControlSchemes() override {
		// The default controller currently in ControlSchemes index 0 is the dynamics one. We push it back to index 2 with this code.

		// Thruster controller

		UHuauvControlThrusters* Thrusters = NewObject<UHuauvControlThrusters>();
		Thrusters->SetController(this);
		ControlSchemes.Insert(Thrusters, 0);

		// Position / orientation controller
		// UHuauvControlPD* PD = NewObject<UHuauvControlPD>();
		// PD->SetController(this);
		// ControlSchemes.Insert(PD, 1);
	}
};

