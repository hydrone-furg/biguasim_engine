#pragma once

#include "CoreMinimal.h"
#include "CameraSensor.h"
#include "GameFramework/Actor.h"
#include "Containers/Queue.h"
#include "Holodeck.h"
#include "RGBDCamera.generated.h"

/**
 * @brief RGB-D (Color + Depth) Camera sensor.
 * Inherits from the UCameraSensor class.
 * Captures both the visible light spectrum and the Z-buffer depth map simultaneously. 
 * Ideal for Visual SLAM and 3D point cloud reconstruction algorithms.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API URGBDCamera : public UCameraSensor
{
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    URGBDCamera();

    /**
     * @brief Initializes the sensor and sets the dual-capture source.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters dynamically.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** @brief Main tick function that retrieves pixels from the render target. */
    virtual void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** @brief Internal counter to manage the capture frequency. */
    int TickCounter = 0;
    
    /** @brief Pointer to the root actor this sensor is attached to. */
    AActor* Parent;
};