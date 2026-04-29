// MIT License (c) 2019 BYU PCCL see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"
#include "RotationSensor.generated.h"

/**
 * @brief Ground Truth Rotation Sensor.
 * Inherits from the HolodeckSensor class.
 * Returns the true absolute rotation of the component it is attached to, 
 * formatted as Euler angles (Roll, Pitch, Yaw) in a 3-element array.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API URotationSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    URotationSensor();

    /**
     * @brief Initializes the sensor and its references.
     */
    virtual void InitializeSensor() override;

protected:
    /** @brief Gets the number of items this sensor returns (3 for Roll, Pitch, Yaw). */
    int GetNumItems() override { return 3; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating and exporting the rotation. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** * @brief Pointer to the root actor this sensor is attached to.
     * Cached after initialization to avoid redundant lookups. Not owned.
     */
    AActor* Parent;

};