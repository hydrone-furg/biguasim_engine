// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "GameFramework/Pawn.h"
#include "HolodeckBuoyantAgent.h"
#include "HoveringAUV.generated.h"

// Changed to constexpr to prevent linker errors if included in multiple .cpp files
constexpr float AUV_MAX_LIN_ACCEL = 10.0f;
constexpr float AUV_MAX_ANG_ACCEL = 2.0f;
constexpr float AUV_MAX_THRUST = AUV_MAX_LIN_ACCEL * 31.02f / 4.0f;

/**
 * @brief Hovering Autonomous Underwater Vehicle (AUV) agent.
 * * Inherits from the HolodeckBuoyantAgent class.
 * On any tick this object will apply the given forces.
 * Desired values must be set by a controller.
 */
UCLASS()
class HOLODECK_API AHoveringAUV : public AHolodeckBuoyantAgent
{
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    AHoveringAUV();

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

    /** @brief Location of all 8 thrusters (4 vertical, 4 vectored). */
    TArray<FVector> thrusterLocations{ FVector(18.18, 22.14, -4), 
                                            FVector(18.18, -22.14, -4),
                                            FVector(-31.43, -22.14, -4),
                                            FVector(-31.43, 22.14, -4),
                                            FVector(7.39, 18.23, -0.21),
                                            FVector(7.39, -18.23, -0.21),
                                            FVector(-20.64, -18.23, -0.21),
                                            FVector(-20.64, 18.23, -0.21) };

    /** @brief Defines if the buoyancy calculation is mathematically idealized. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BuoyancySettings)
    bool Perfect = true;

    /**
     * @brief Applies specific force values to the AUV's thrusters.
     * @param ThrusterArray Pointer to an array containing the requested force for each thruster.
     */
    void ApplyThrusters(float* const ThrusterArray);

    /**
     * @brief Enables linear and angular damping for the AUV's physical mesh.
     */
    void EnableDamping();

private:
    /** @brief Internal array that stores the 6-DOF accelerations/forces received from Python. */
    float CommandArray[6];

};