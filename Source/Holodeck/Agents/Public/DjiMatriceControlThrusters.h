#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "DjiMatrice.h"
#include "HolodeckControlScheme.h"
#include "SimplePID.h"
#include <math.h>

#include "DjiMatriceControlThrusters.generated.h"

/**
* UDjiMatriceControlThrusters
*/
UCLASS()
class HOLODECK_API UDjiMatriceControlThrusters : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	UDjiMatriceControlThrusters(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	/** NOTE: These go counter-clockwise, starting in front right

	*/
	unsigned int GetControlSchemeSizeInBytes() const override {
		return 8 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { DjiMatriceController = Controller; };

private:
	AHolodeckPawnController* DjiMatriceController;
	ADjiMatrice* DjiMatrice;

};

