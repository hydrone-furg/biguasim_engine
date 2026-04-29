// MIT License (c) 2019 BYU PCCL see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSensor.h"
#include "RangeFinderSensor.generated.h"

/**
 * @brief Range Finder Sensor (1D to 2D Lidar).
 * Inherits from the HolodeckSensor class.
 * Gets distances to the first collision. By default, returns a single float representing 
 * the distance in the agent's forward direction. Increasing LaserCount creates an array 
 * of rays evenly distributed 360 degrees in a plane around the agent. LaserAngle offsets 
 * each of these rays, transforming the plane into a conical shape. 
 */
UCLASS()
class HOLODECK_API URangeFinderSensor : public UHolodeckSensor
{
    GENERATED_BODY()
    
public:
    /**
     * @brief Default Constructor.
     */
    URangeFinderSensor();

    /**
     * @brief Initializes the sensor and caches the root actor.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set laser count, angle, max distance, and debug flags.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** @brief Gets the number of items this sensor returns (equal to LaserCount). */
    int GetNumItems() override { return LaserCount; };
    
    /** @brief Gets the size of each item in bytes (float). */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating raycasts and distances. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Number of laser rays to cast. 1 is a single forward beam, higher numbers create a 360 sweep. */
    UPROPERTY(EditAnywhere)
    int LaserCount = 1;

    /** @brief Pitch angle of the lasers. Adjusting this transforms a flat 360 plane into a cone. */
    UPROPERTY(EditAnywhere)
    float LaserAngle = 0;

    /** @brief Maximum distance each laser can travel before returning max range (in cm). */
    UPROPERTY(EditAnywhere)
    float LaserMaxDistance = 1000;

    /** @brief Toggles drawing the laser rays in the Unreal Editor for debugging. */
    UPROPERTY(EditAnywhere)
    bool LaserDebug = false;

private:
    /** * @brief Pointer to the root actor this sensor is attached to. 
     * Used to ignore self-collisions during raycasting.
     */
    AActor* Parent;
};