// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSonar.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"
#include "Async/ParallelFor.h"
#include "MultivariateNormal.h"
#include "MultivariateUniform.h"

#include <numeric>
#include "Json.h"

#include "ImagingSonar.generated.h"

#define Pi 3.1415926535897932384626433832795

/**
 * @brief Forward Looking Imaging Sonar (FLS) sensor simulation.
 * Emits an acoustic wave, calculates intersections using an Octree, and simulates 
 * complex acoustic phenomena including multipath reflections, speckle noise, 
 * and acoustic shadowing. Supports generating ground-truth point clouds and elevation maps.
 */
UCLASS()
class HOLODECK_API UImagingSonar : public UHolodeckSonar
{
    GENERATED_BODY()
    
public:
    /**
     * @brief Default Constructor.
     */
    UImagingSonar();

    /**
     * @brief Initializes the sensor matrices and computes shadowing scales.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses configuration settings from a JSON payload.
     * @param ParmsJson JSON string containing noise parameters, resolutions, and multipath flags.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

    /**
     * @brief Cleans up memory arrays allocated for the octree and binning.
     */
    virtual void BeginDestroy() override;

protected:
    /** @brief Gets the number of rows (Range Bins) for the output buffer. */
    int GetRows() const override { return RangeBins; }
    
    /** @brief Gets the number of columns (Azimuth Bins) for the output buffer. */
    int GetCols() const override { return AzimuthBins; }

    /** @brief Determines the byte size of each item in the buffer (float). */
    int GetItemSize() override { return sizeof(float); };

    /**
     * @brief The core processing loop to calculate acoustics and noise per frame.
     */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Number of bins used to slice the maximum range distance. */
    UPROPERTY(EditAnywhere)
    int32 RangeBins = 0;

    /** @brief Resolution of each range bin in centimeters. */
    UPROPERTY(EditAnywhere)
    float RangeRes = 0;

    /** @brief Number of horizontal sweeping bins. */
    UPROPERTY(EditAnywhere)
    int32 AzimuthBins = 0;

    /** @brief Degree resolution per azimuth bin. */
    UPROPERTY(EditAnywhere)
    float AzimuthRes = 0;

    /** @brief Number of vertical sweeping bins for acoustic propagation. */
    UPROPERTY(EditAnywhere)
    int32 ElevationBins = 0;

    /** @brief Degree resolution per elevation bin. */
    UPROPERTY(EditAnywhere)
    float ElevationRes = 0;

    /** @brief Flag to enable simulating secondary multipath acoustic bounces. */
    UPROPERTY(EditAnywhere)
    bool MultiPath = false;

    /** @brief Size of the cluster area when calculating multipath reflections. */
    UPROPERTY(EditAnywhere)
    int32 ClusterSize = 5;

    /** @brief Flag to scale noise logarithmically by distance and angle. */
    UPROPERTY(EditAnywhere)
    bool ScaleNoise = true;

    /** @brief Flag to simulate hardware defects (streaks) along specific azimuth angles. */
    UPROPERTY(EditAnywhere)
    int32 AzimuthStreaks = 0;

private:
    /** @brief Pointer to the actor this sensor is attached to. */
    AActor* Parent;

    // Various computations we want to cache
    /** @brief Precomputed scale to optimize shadowing checks. */
    int32 AzimuthBinScale = 1;
    
    /** @brief Cached cosine value for determining a perfect perpendicular reflection. */
    float perfectCos;

    // Used to hold leaves for multipath
    TMap<FIntVector,Octree*> mapLeaves;
    TMap<FIntVector,Octree*> mapSearch;
    TArray<TArray<Octree*>> cluster;
    int32* count;
    int32* hasPerfectNormal;
    
    // Noise generation objects
    MultivariateNormal<1> addNoise;
    MultivariateNormal<1> multNoise;
    MultivariateUniform<1> rNoise;
};