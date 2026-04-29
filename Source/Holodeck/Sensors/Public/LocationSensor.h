// MIT License (c) 2019 BYU PCCL see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"
#include "MultivariateNormal.h"
#include "LocationSensor.generated.h"

/**
 * @brief Ground Truth Location Sensor.
 * Inherits from the HolodeckSensor class.
 * Reports the absolute XYZ coordinate of the parent agent in the global frame.
 * Unlike the GPS sensor, this sensor does not lose signal underwater and acts 
 * as a perfect (or noise-injected) ground truth tracker.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API ULocationSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    ULocationSensor();

    /**
     * @brief Initializes the sensor and its references.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set noise limits.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** @brief Gets the number of items this sensor returns (3 for X, Y, Z). */
    int GetNumItems() override { return 3; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating absolute position. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** * @brief Pointer to whatever the sensor is attached to. 
     * Used to extract the raw world location. Not owned.
     */
    USceneComponent* Parent;
    
    /** @brief 3D multivariate normal distribution for generating optional positional noise. */
    MultivariateNormal<3> mvn;
};