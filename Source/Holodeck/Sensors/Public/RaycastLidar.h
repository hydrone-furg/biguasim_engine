// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include <vector>

#include "Holodeck.h"
#include "HolodeckSensor.h"
#include "LidarData.h"
#include "LidarDescription.h"
#include "RandomEngine.h"
#include "RaycastSemanticLidar.h"
#include "SemanticLidarData.h"

#include "RaycastLidar.generated.h"

/**
 * @brief High-fidelity Raycast Lidar sensor.
 * Inherits from URaycastSemanticLidar.
 * Simulates a rotating 3D Lidar scanner with realistic physical properties, including 
 * atmospheric attenuation, Gaussian distance noise, and intensity-based point drop-off.
 * Originates from the CARLA Simulator physics engine.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API URaycastLidar : public URaycastSemanticLidar
{
    GENERATED_BODY()

    using FLidarData = holoocean::data::LidarData;
    using FDetection = holoocean::data::LidarDetection;

public:
    /**
     * @brief Default Constructor.
     */
    URaycastLidar();    

    /**
     * @brief Initializes the sensor and sets up data buffers.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to configure the Lidar's physical and noise properties.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

    /**
     * @brief Applies the parsed description parameters to the sensor.
     * @param LidarDescription The parsed lidar configuration struct.
     */
    virtual void Set(const FLidarDescription& LidarDescription) override;

protected:
    /** @brief Main execution loop that fires the Lidar simulation. */
    virtual void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    /** @brief Gets the maximum possible number of items this sensor returns (PointsPerSecond * 4 elements per point). */
    int virtual GetNumItems() override { return LidarDescription.PointsPerSecond * 4; };
    
    /** @brief Gets the size of each item in bytes. */
    int virtual GetItemSize() override { return sizeof(float); };

    float* Buffer;
    FLidarData Data;
    
private:
    FSemanticLidarData SemanticLidarData;
    FLidarDescription LidarDescription;

    /// Compute the received intensity of the point
    float ComputeIntensity(const FSemanticDetection& RawDetection) const;
    
    /// Compute a valid detection from a physics hit result
    FDetection ComputeDetection(const FHitResult& HitInfo, const FTransform& SensorTransf) const;

    virtual void PreprocessRays(uint32_t Channels, uint32_t MaxPointsPerChannel) override;
    bool PostprocessDetection(FDetection& Detection) const;

    virtual void ComputeAndSaveDetections(const FTransform& SensorTransform) override;

    FLidarData TempData;

    /// Enable/Disable general drop-off of lidar points
    bool DropOffGenActive;

    /**
     * @brief Slope for the intensity drop-off of lidar points.
     * Calculated through the drop-off limit and the drop-off at zero intensity.
     * The point is kept with a probability of: alpha * Intensity + beta
     * where:
     * alpha = (1 - dropoff_zero_intensity) / dropoff_limit
     * beta = (1 - dropoff_zero_intensity)
     */
    float DropOffAlpha;
    float DropOffBeta;
    
    /** @brief Unique pointer to the root actor this sensor is attached to. */
    TUniquePtr<AActor> Parent;
};