// MIT License (c) 2021 BYU FRoStLab see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"

#include "MultivariateNormal.h"
#include "Kismet/KismetMathLibrary.h"

#include "DVLSensor.generated.h"

/**
 * @brief Doppler Velocity Log (DVL) acoustic sensor.
 * Inherits from the HolodeckSensor class.
 * Emits 4 simulated acoustic beams (Janus configuration) to estimate the true 
 * linear velocity of the attached agent relative to the environment. Can optionally 
 * perform raycasts to return the altitude/range of each beam.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UDVLSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UDVLSensor();

    /**
     * @brief Initializes the sensor and pre-computes the Janus transformation matrices.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set elevation, max range, and multivariate noise profiles.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** * @brief Gets the number of items returned.
     * @return 7 items if ReturnRange is true (3 velocities + 4 beam ranges), otherwise 3 items. 
     */
    int GetNumItems() override { return ReturnRange ? 7 : 3; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating velocity and performing raycasts. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Toggles drawing the 4 acoustic beams in the Unreal Editor for debugging. */
    UPROPERTY(EditAnywhere)
    bool DebugLines = false;

    /** @brief The angle (in degrees) of the acoustic beams relative to the horizontal plane. */
    UPROPERTY(EditAnywhere)
    float elevation = 22.5;

    /** @brief Determines if the sensor should raycast and return the distances of the 4 beams. */
    UPROPERTY(EditAnywhere)
    bool ReturnRange = true;

    /** @brief The maximum operational distance of the acoustic beams (in cm). */
    UPROPERTY(EditAnywhere)
    float MaxRange = 20 * 100;

private:
    /** * @brief Pointer to whatever the sensor is attached to. 
     * Used to extract the raw physics velocity. Not owned.
     */
    UPrimitiveComponent* Parent;

    // Used for noise and mathematical transforms
    float sinElev;
    float cosElev;
    MultivariateNormal<4> mvnVel;
    MultivariateNormal<4> mvnRange;
    TArray<TArray<float>> transform;

    /** @brief Vector directions for the 4 acoustic beams. */
    TArray<FVector> directions;
};