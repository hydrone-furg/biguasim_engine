// MIT License (c) 2019 BYU PCCL see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "GameFramework/Pawn.h"
#include "HolodeckBuoyantAgent.h"
#include "TorpedoAUV.generated.h"

// Changed to constexpr to prevent linker errors if included in multiple .cpp files
constexpr float TAUV_MIN_THRUST = -100.0f;
constexpr float TAUV_MAX_THRUST = 100.0f;
constexpr float TAUV_MIN_FIN = -45.0f;
constexpr float TAUV_MAX_FIN = 45.0f;

/**
 * @brief Torpedo-style Autonomous Underwater Vehicle (AUV) agent.
 * Inherits from the HolodeckBuoyantAgent class.
 * On any tick this object will apply the given forces.
 * Desired values must be set by a controller.
 */
UCLASS()
class HOLODECK_API ATorpedoAUV : public AHolodeckBuoyantAgent
{
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    ATorpedoAUV();

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
     * Allows agent to fall up to ~8 meters.
     * @return The acceleration limit.
     */
    float GetAccelerationLimit() override { return 200; }

    /** @brief Location of the main rear thruster force application point. */
    FVector thruster = FVector(-120, 0, 0);

    /** @brief Locations of the 4 control fins. */
    TArray<FVector> finTranslation{ FVector(-105, 7.07, 0),
                                    FVector(-105, 0, 7.07),
                                    FVector(-105, -7.07, 0),
                                    FVector(-105, 0, -7.07) };

    /** @brief Rotations of the 4 control fins. */
    TArray<FRotator> finRotation{   FRotator(0, 0, 0),
                                    FRotator(0, 0, -90),
                                    FRotator(0, 0, -180),
                                    FRotator(0, 0, -270) };

    /**
     * @brief Calculates and applies lift and drag forces for a specific fin.
     * @param i The index of the fin (0 to 3).
     * @param command The commanded angle for the fin.
     */
    void ApplyFin(int i, float command);

    /**
     * @brief Applies linear thrust from the main rear propeller.
     * @param thrust The thrust value to apply.
     */
    void ApplyThrust(float thrust);

    /**
     * @brief Enables linear and angular damping for the AUV's physical mesh.
     */
    void EnableDamping();

private:
    /** @brief Internal array that stores the 6-DOF accelerations/forces received from Python. */
    float CommandArray[6];

};