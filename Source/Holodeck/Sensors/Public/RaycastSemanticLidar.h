// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include <vector>

#include "Holodeck.h"
#include "HolodeckSensor.h"

#include "LidarDescription.h"
#include "RandomEngine.h"
#include "SemanticLidarData.h"

DECLARE_STATS_GROUP(TEXT("HoloOceanRaycasting"), STATGROUP_RaycastFunction, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("CopyBuffer"), STAT_CopyBuffer, STATGROUP_RaycastFunction);
DECLARE_CYCLE_STAT(TEXT("ParallelFor"), STAT_ParallelForHoloocean, STATGROUP_RaycastFunction);

#include "RaycastSemanticLidar.generated.h"

/**
 * @brief Semantic Raycast Lidar sensor.
 * Inherits from UHolodeckSensor.
 * Simulates a rotating 3D Lidar scanner capable of extracting semantic IDs 
 * (CustomDepthStencilValue) from the geometry it hits. Essential for 
 * training 3D semantic segmentation networks. Originates from the CARLA Simulator.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API URaycastSemanticLidar : public UHolodeckSensor
{
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    URaycastSemanticLidar();    

    /**
     * @brief Initializes the sensor and sets up data buffers.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to configure the Lidar's physical properties.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;
    
    /**
     * @brief Applies the parsed description parameters to the sensor.
     * @param LidarDescription The parsed lidar configuration struct.
     */
    virtual void Set(const FLidarDescription& LidarDescription);

protected:

    using FSemanticLidarData = holoocean::data::SemanticLidarData;
    using FSemanticDetection = holoocean::data::SemanticLidarDetection;
    
    /** @brief Gets the maximum possible number of items this sensor returns. */
    int virtual GetNumItems() override { return Description.PointsPerSecond; };
    
    /** @brief Gets the size of each item in bytes. */
    int virtual GetItemSize() override { return sizeof(float); };
    
    /** @brief Main execution loop that fires the Lidar simulation. */
    void virtual TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * @brief Creates a Laser angle profile for each channel.
     */
    void CreateLasers();

    /**
     * @brief Updates LidarMeasurement with the points read in DeltaTime.
     */
    void SimulateLidar(const float DeltaTime);

    /**
     * @brief Shoots a laser ray-trace, return whether the laser hit something.
     */
    bool ShootLaser(const float VerticalAngle, float HorizontalAngle, FHitResult &HitResult, FCollisionQueryParams& TraceParams) const;

    /**
     * @brief Method that allows preprocessing if the rays will be traced.
     */
    virtual void PreprocessRays(uint32_t Channels, uint32_t MaxPointsPerChannel);

    /**
     * @brief Compute all raw detection information, including semantic tags.
     */
    void ComputeRawDetection(const FHitResult &HitInfo, const FTransform& SensorTransf, FSemanticDetection& Detection) const;

    /**
     * @brief Saves the hits the raycast returns per channel asynchronously.
     */
    void WritePointAsync(uint32_t Channel, FHitResult &Detection);

    /**
     * @brief Clears the recorded data structure for the next frame.
     */
    void ResetRecordedHits(uint32_t Channels, uint32_t MaxPointsPerChannel);

    /**
     * @brief Uses all saved FHitResults, computes RawDetections, and writes to shared memory.
     */
    virtual void ComputeAndSaveDetections(const FTransform &SensorTransform);

    UPROPERTY(EditAnywhere)
    FLidarDescription Description;

    TArray<float> LaserAngles;

    std::vector<std::vector<FHitResult>> RecordedHits;
    std::vector<std::vector<bool>> RayPreprocessCondition;
    std::vector<uint32_t> PointsPerChannel;

    /// Random Engine used to provide noise for sensor output.
    UPROPERTY(VisibleAnywhere)
    URandomEngine* RandomEngine = nullptr;
    
    bool ShouldInitialize = true;

private:
    /** @brief Unique pointer to the root actor this sensor is attached to. */
    TUniquePtr<AActor> Parent;

    FSemanticLidarData SemanticLidarData;
    float* SemanticLidarBuffer;

};