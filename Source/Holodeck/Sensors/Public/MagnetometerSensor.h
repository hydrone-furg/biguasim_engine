// MIT License (c) 2021 BYU FRoStLab see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"
#include <limits>
#include "MultivariateNormal.h"
#include "MagnetometerSensor.generated.h"

/**
 * @brief Simulated Magnetometer (3-Axis Compass).
 * Inherits from the HolodeckSensor class.
 * Reports the global magnetic vector (by default, the global X-axis) translated 
 * into the sensor's local coordinate frame. Crucial for determining absolute heading/yaw.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UMagnetometerSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UMagnetometerSensor();

    /**
     * @brief Initializes the sensor and its references.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set the magnetic vector and noise characteristics.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** @brief Gets the number of items this sensor returns (3 for X, Y, Z vector). */
    int GetNumItems() override { return 3; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating the local magnetic vector. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief The global vector representing the magnetic field (Default is Unreal's X-Axis / Forward). */
    UPROPERTY(EditAnywhere)
    FVector MeasuredVector = FVector(1, 0, 0);

private:
    /** * @brief Pointer to whatever the sensor is attached to. 
     * Used to extract the raw world rotation. Not owned.
     */
    USceneComponent* Parent;
    
    /** @brief 3D multivariate normal distribution for generating noise in the magnetic reading. */
    MultivariateNormal<3> mvn;
};