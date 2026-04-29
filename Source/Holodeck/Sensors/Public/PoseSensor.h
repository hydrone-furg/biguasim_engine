// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"
#include "HolodeckPawnController.h"
#include "HolodeckSensor.h"
#include "PoseSensor.generated.h"

/**
 * @brief Absolute Pose Sensor.
 * Inherits from the HolodeckSensor class.
 * Returns the complete pose (position and orientation) of the parent agent 
 * packed into a 16-element array, representing a 4x4 Homogeneous Transformation Matrix.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOLODECK_API UPoseSensor : public UHolodeckSensor {
    GENERATED_BODY()

public: 
    /**
     * @brief Default Constructor.
     */
    UPoseSensor();

    /**
     * @brief Initializes the sensor and caches necessary components.
     */
    virtual void InitializeSensor() override;
    
protected:
    /** @brief Gets the number of items this sensor returns (16 floats for a 4x4 matrix). */
    int GetNumItems() override { return 16; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating the Transformation Matrix. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** @brief Pointer to the parent primitive component. */
    UPrimitiveComponent* Parent;
    
    /** @brief Pointer to the parent's static mesh component. */
    UStaticMeshComponent* RootMesh;
    
    /** @brief Pointer to the current game world. */
    UWorld* World;
};