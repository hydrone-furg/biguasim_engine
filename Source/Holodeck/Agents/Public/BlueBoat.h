// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "GameFramework/Pawn.h"
#include "HolodeckBuoyantAgent.h"
#include "BlueBoat.generated.h"

// Changed to constexpr to prevent linker errors if included in multiple .cpp files
constexpr float BB_MAX_LIN_ACCEL = 20.0f;
constexpr float BB_MAX_ANG_ACCEL = 2.0f;
constexpr float BB_MAX_THRUST = 1500.0f;

/**
 * @brief Surface vessel agent (BlueBoat) for multi-domain simulation.
 * * Inherits from the HolodeckBuoyantAgent class.
 * On any tick this object will apply the given forces.
 * Desired values must be set by a controller.
 */
UCLASS()
class HOLODECK_API ABlueBoat : public AHolodeckBuoyantAgent
{
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    ABlueBoat();

    /**
     * @brief Initializes the agent and its physical components.
     */
    void InitializeAgent() override;

    /**
     * @brief Called each frame.
     * @param DeltaSeconds The time since the last tick.
     */
    void Tick(float DeltaSeconds) override;

    /**
     * @brief Gets the size of the action buffer in bytes.
     * @return The size of the command array in bytes (6 floats).
     */
    unsigned int GetRawActionSizeInBytes() const override { return 6 * sizeof(float); };

    /**
     * @brief Gets a pointer to the raw action buffer.
     * @return A void pointer to the command array.
     */
    void* GetRawActionBuffer() const override { return (void*)CommandArray; };

    /**
     * @brief Gets the acceleration limit for the agent.
     * * Allows agent to fall up to ~8 meters.
     * @return The acceleration limit.
     */
    float GetAccelerationLimit() override { return 400; }

    /** @brief Location of all thrusters - Left and Right */
    TArray<FVector> thrusterLocations{ FVector(-250, -100, 0), FVector(-250, 100, 0) };

    /** @brief Defines if the buoyancy calculation is perfect or approximated. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BuoyancySettings)
    bool Perfect = true;

    /**
     * @brief Applies specific force values to the boat's thrusters.
     * @param ThrusterArray Pointer to an array containing the requested force for each thruster.
     */
    void ApplyThrusters(float* const ThrusterArray);

    /**
     * @brief Enables linear and angular damping for the boat's physical mesh.
     */
    void EnableDamping();

private:
    /** @brief Internal array that stores the forces received from Python. */
    float CommandArray[6];

};