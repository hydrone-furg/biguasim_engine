// MIT License (c) 2019 BYU PCCL see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"
#include "VelocitySensor.generated.h"

/**
 * @brief Ground Truth Velocity Sensor.
 * Inherits from the HolodeckSensor class.
 * Gets the true absolute linear velocity of the component that the sensor is attached to.
 * Returns a 3-element array representing the X, Y, and Z velocity vectors.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UVelocitySensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UVelocitySensor();

    /**
     * @brief Initializes the sensor and caches the parent component.
     */
    virtual void InitializeSensor() override;

protected:
    /** @brief Gets the number of items this sensor returns (3 for X, Y, Z velocity). */
    int GetNumItems() override { return 3; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating and exporting the linear velocity. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /**
     * @brief Pointer to the primitive component this sensor is attached to.
     * Cached after initialization to avoid redundant lookups. Not owned.
     */
    UPrimitiveComponent* Parent;
};