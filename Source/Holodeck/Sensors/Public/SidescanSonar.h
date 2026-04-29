// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSonar.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"
#include "Async/ParallelFor.h"
#include "MultivariateNormal.h"
#include "Json.h"

#include "SidescanSonar.generated.h"

#define Pi 3.1415926535897932384626433832795

/**
 * @brief Side-scan Sonar simulation.
 * Inherits from the UHolodeckSonar class.
 * Emits a wide horizontal but extremely narrow vertical acoustic pulse to map the seafloor.
 * Unlike imaging sonars, this sensor returns a 1D array representing a single slice of 
 * port and starboard acoustic returns per tick.
 */
UCLASS()
class HOLODECK_API USidescanSonar : public UHolodeckSonar
{
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
   USidescanSonar();

    /**
     * @brief Initializes the sensor matrices and 1D bin arrays.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters, enforcing side-scan specific default profiles.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

    /**
     * @brief Cleans up memory arrays allocated for binning.
     */
    virtual void BeginDestroy() override;

protected:
    /** @brief Gets the number of rows (Range Bins). */
    int GetRows() const override { return RangeBins; }
    
    /** @brief Gets the number of columns. Forced to 1 to create a 1D output array. */
    int GetCols() const override { return 1; }

    /** @brief Gets the size of each item in bytes (float). */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main acoustic processing and data formatting loop. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Number of bins used to slice the maximum range distance. */
    UPROPERTY(EditAnywhere)
    int32 RangeBins = 0;

    /** @brief Resolution of each range bin. */
    UPROPERTY(EditAnywhere)
    float RangeRes = 0;

    /** @brief Number of horizontal sweeping bins (port to starboard). */
    UPROPERTY(EditAnywhere)
    int32 AzimuthBins = 0;

    /** @brief Degree resolution per azimuth bin. */
    UPROPERTY(EditAnywhere)
    float AzimuthRes = 0;

    /** @brief Number of vertical bins. Usually extremely small for Sidescan. */
    UPROPERTY(EditAnywhere)
    int32 ElevationBins = 0;

    /** @brief Degree resolution per elevation bin. */
    UPROPERTY(EditAnywhere)
    float ElevationRes = 0;

private:
    /** @brief Pointer to the actor this sensor is attached to. */
    AActor* Parent;

    /** @brief Used for counting how many leaves fall into a bin to calculate averages. */
    int32* count;
    
    /** @brief Internal counter to trigger periodic editor warnings. */
    uint32 runtickCounter = 0;
    
    // Noise generation profiles
    MultivariateNormal<1> addNoise;
    MultivariateNormal<1> multNoise;
};