// MIT License (c) 2019 BYU PCCL see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckCamera.h"
#include "RGBCamera.generated.h"

/**
 * @brief Standard RGB Camera sensor.
 * Inherits from the HolodeckCamera class.
 * Captures standard visible-light images (FinalColorLDR) from the environment, 
 * simulating a typical optical camera.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API URGBCamera : public UHolodeckCamera {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    URGBCamera();

    /**
     * @brief Initializes the sensor and sets the capture source.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set the capture rate.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

    /** @brief How many engine ticks must pass before a new frame is captured. */
    UPROPERTY(EditAnywhere)
    int TicksPerCapture = 1;

protected:
    /** @brief Main tick function that retrieves pixels from the render target. */
    virtual void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Gets the number of items this sensor returns (Pixels = Width * Height). */
    virtual int GetNumItems() override { return CaptureWidth * CaptureHeight; };
    
    /** @brief Gets the size of each item in bytes. */
    virtual int GetItemSize() override { return sizeof(float); };
    
private:
    /** @brief Internal counter to manage capture frequency. */
    int TickCounter = 0;

};