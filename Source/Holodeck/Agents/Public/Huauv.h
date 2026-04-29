// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "GameFramework/Pawn.h"
#include "HolodeckBuoyantAgent.h"
#include "Huauv.generated.h"

const float H_MAX_LIN_ACCEL = 10;
const float H_MAX_ANG_ACCEL = 2;
const float H_MAX_THRUST = H_MAX_LIN_ACCEL*11.5 / 4;
const float AIR_MAX_ROLL = 6.5080;
const float AIR_MAX_PITCH = 5.087;
const float AIR_MAX_YAW_RATE = .8;
const float AIR_MAX_FORCE = 5984.4;

UCLASS()
/**
* Huauv
* Inherits from the HolodeckAgent class
* On any tick this object will apply the given forces.
* Desired values must be set by a controller.
*/
class HOLODECK_API AHuauv : public AHolodeckBuoyantAgent
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor.
	*/
	AHuauv();

	void InitializeAgent() override;

	/**
	* Tick
	* Called each frame.
	* @param DeltaSeconds the time since the last tick.
	*/

	void Tick(float DeltaSeconds) override;

	unsigned int GetRawActionSizeInBytes() const override { return 8 * sizeof(float); };
	void* GetRawActionBuffer() const override { return (void*)CommandArray; };

	// Allows agent to fall up to ~8 meters
	float GetAccelerationLimit() override { return 400; }

	// Location of all thrusters
	TArray<FVector> thrusterLocations{ FVector(18.18, 22.14, -4), 
										FVector(18.18, -22.14, -4),
										FVector(-31.43, -22.14, -4),
										FVector(-31.43, 22.14, -4),
										FVector(7.39, 18.23, -0.21),
										FVector(7.39, -18.23, -0.21),
										FVector(-20.64, -18.23, -0.21),
										FVector(-20.64, 18.23, -0.21) };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BuoyancySettings)
		bool Perfect= true;

	void ApplyAirThrusters(float* const ThrusterArray);
	void ApplyUnderwaterThrusters(float* const ThrusterArray);

	void EnableDamping();

private:
	// Accelerations
	float CommandArray[8];

};
