// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "DjiMatriceControlThrusters.h"
// #include "DjiMatricePD.h"
#include "DjiMatrice.h"


#include "DjiMatriceController.generated.h"

/**
* A Holodeck Turtle Agent Controller
*/
UCLASS()

class HOLODECK_API ADjiMatriceController : public AHolodeckPawnController
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	ADjiMatriceController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Default Destructor
	*/
	~ADjiMatriceController();

	void AddControlSchemes() override {
		// The default controller currently in ControlSchemes index 0 is the dynamics one. We push it back to index 1 with this code.

		// Thruster controller

		UDjiMatriceControlThrusters* Thrusters = NewObject<UDjiMatriceControlThrusters>();
		Thrusters->SetController(this);
		ControlSchemes.Insert(Thrusters, 0);

		// // Position / orientation controller
		// UDjiMatricePD* PD = NewObject<UDjiMatricePD>();
		// PD->SetController(this);
		// ControlSchemes.Insert(PD, 1);
	}
};

