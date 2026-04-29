// MIT License (c) 2021 BYU FRoStLab see LICENSE file
#pragma once

#include "Holodeck.h"

#include "HolodeckSensor.h"

#include <limits>
#include "MultivariateNormal.h"

#include "GPSSensor.generated.h"

/**
 * @brief Simulated Global Positioning System (GPS) sensor.
 * Inherits from the HolodeckSensor class.
 * Reports the XYZ coordinate of the parent agent in the global frame. 
 * Realistically simulates signal loss underwater by returning NaN when the 
 * sensor is submerged beyond a defined depth threshold.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UGPSSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UGPSSensor();

    /**
     * @brief Initializes the sensor and its references.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set noise limits and operational depth.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** @brief Gets the number of items this sensor returns (3 for X, Y, Z). */
    int GetNumItems() override { return 3; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating position and handling depth signal loss. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Maximum depth (in meters) before the GPS loses signal and returns NaNs. */
    UPROPERTY(EditAnywhere)
    float GPSDepth = 2;

private:
    /** * @brief Pointer to whatever the sensor is attached to. 
     * Used to extract the raw world location. Not owned.
     */
    USceneComponent* Parent;
    
    /** @brief 3D multivariate normal distribution for generating positional noise (XYZ). */
    MultivariateNormal<3> mvn;
    
    /** @brief 1D multivariate normal distribution for generating noise in depth estimation. */
    MultivariateNormal<1> depthMVN;
};