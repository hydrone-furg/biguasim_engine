#pragma once

#include "CameraSensor.h"
#include "CoreMinimal.h"
#include "Holodeck.h"
#include "DepthCamera.generated.h"

/**
 * @brief Custom high-fidelity Z-buffer depth camera.
 * Inherits from the UCameraSensor class. 
 * Captures pure geometric depth where each pixel represents a precise straight ray, 
 * rather than an expanding cone like an acoustic sonar. Ideal for point cloud generation.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UDepthCamera : public UCameraSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UDepthCamera();

    /**
     * @brief Initializes the sensor rendering parameters and disables visual artifacts.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set resolution and capture rate.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

    /**
     * @brief Gets the size of each pixel data structure.
     * @return Data size in bytes.
     */
    virtual int GetItemSize() override;

    /**
     * @brief Gets the total number of pixels in the capture frame.
     * @return Total items.
     */
    virtual int GetNumItems() override;

protected:
    /**
     * @brief Executes the render extraction command and writes to shared memory.
     */
    virtual void TickSensorComponent(
        float                        DeltaTime,
        ELevelTick                   TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** @brief Internal counter to manage TicksPerCapture logic. */
    int TickCounter = 0;
    
    /** @brief Internal buffer holding the raw 16-bit float colors before conversion. */
    TArray<FFloat16Color> FloatBuffer;
};