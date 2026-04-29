// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "GameFramework/Pawn.h"
#include "HolodeckAgent.h"
#include "DjiMatrice.generated.h"

// External Forces
constexpr float M_AIR_DENSITY = 1225.0f; // N/m^3
constexpr float M_DRAG_COEFF = 0.8f;

// Body force limits
constexpr float M_MAX_LIN_ACCEL = 20.0f;
constexpr float M_MAX_ANG_ACCEL = 2.0f;

/**
 * @brief Unmanned Aerial Vehicle (UAV) agent representing a DJI Matrice drone.
 * * Inherits from the HolodeckAgent class (Note: not buoyant).
 * On any tick this object will apply the given forces.
 * Desired values must be set by a controller.
 */
UCLASS()
class HOLODECK_API ADjiMatrice : public AHolodeckAgent {
    GENERATED_BODY()

public:
    /** @brief The visual and physical static mesh component of the drone. */
    UPROPERTY(BlueprintReadWrite, Category = UAVMesh)
    UStaticMeshComponent* RootMesh;

    /** @brief Default Constructor. */
    ADjiMatrice();

    /** @brief Initializes the agent and calculates the rotor arm length. */
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

    /** @brief Location of all 4 rotors. */
    TArray<FVector> thrusterLocations{  FVector(28, 32, -5), 
                                        FVector(28, -32, -5),
                                        FVector(-28, 32, -5),
                                        FVector(-28, -32, -5)};

    /** @brief Defines if idealized physics should be used. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BuoyancySettings)
    bool Perfect = true;

    /**
     * @brief Applies thrust to the 4 rotors based on a percentage array.
     * @param ThrusterArray Pointer to an array containing the requested percentage for each rotor.
     */
    void ApplyThrusters(float* const ThrusterArray);
    
    /**
     * @brief Applies aerodynamic drag and gravity to the UAV.
     */
    void ApplyFlightForce();

    /**
     * @brief Polynomial curve calculating Rotor RPM based on command.
     * @param x Command percentage.
     * @return RPM value.
     */
    float GetRPM(float x) { return -51.8f + 81.9f*x - 0.25f*pow(x,2); };

    /**
     * @brief Polynomial curve calculating Rotor Thrust based on command.
     * @param x Command percentage.
     * @return Thrust force.
     */
    float GetThrust(float x) { return -2170.0f + 116.0f*x - 0.399f*pow(x,2); };

    /**
     * @brief Toggles whether forces are calculated in the main controller loop.
     * @param SetIt Boolean flag to set the state.
     */
    void ForcesInMainController(bool SetIt);

private:
    /** @brief Array storing the linear and angular accelerations. */
    float CommandArray[6];
    
    /** @brief Flag indicating if forces are processed in the main tick. */
    bool bForcesInMain;

    float Volume; // in m^3
    float MassInKG;
    float Gravity;
    FVector CenterMass; // in cm

    /** @brief Calculated distance from the drone's center to a rotor. */
    float ArmLength;
};