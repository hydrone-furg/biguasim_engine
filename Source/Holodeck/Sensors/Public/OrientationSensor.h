// MIT License (c) 2019 BYU PCCL see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckPawnController.h"
#include "HolodeckSensor.h"
#include "OrientationSensor.generated.h"

/**
 * @brief Absolute Orientation Sensor.
 * Inherits from the HolodeckSensor class.
 * Gives the complete orientation of the parent agent in three directional vectors: 
 * forward, left, and up. Effectively returns a 3x3 rotation matrix.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOLODECK_API UOrientationSensor : public UHolodeckSensor {
    GENERATED_BODY()

public: 
    /**
     * @brief Default Constructor.
     */
    UOrientationSensor();

    /**
     * @brief Initializes the sensor and caches necessary components.
     */
    virtual void InitializeSensor() override;
    
protected:
    /** @brief Gets the number of items this sensor returns (9 floats for a 3x3 matrix). */
    int GetNumItems() override { return 9; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating the Forward, Left, and Up vectors. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** @brief Pointer to the parent primitive component. */
    UPrimitiveComponent* Parent;
    
    /** @brief Pointer to the parent's static mesh component. */
    UStaticMeshComponent* RootMesh;
    
    /** @brief Pointer to the current game world. */
    UWorld* World;
};