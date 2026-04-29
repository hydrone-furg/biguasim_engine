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
#include "Json.h"
#include "SinglebeamSonar.generated.h"

#define Pi 3.1415926535897932384626433832795

/**
 * @brief Singlebeam Sonar (Echosounder) simulation.
 * Inherits from the UHolodeckSonar class.
 * Emits a single conical acoustic pulse to measure distance and return intensity. 
 * commonly pointed downwards to measure water depth/clearance. Returns a 1D array 
 * of intensities along the range of the cone.
 */
UCLASS()
class HOLODECK_API USinglebeamSonar : public UHolodeckSonar
{
    GENERATED_BODY()
    
public:
    /**
     * @brief Default Constructor.
     */
    USinglebeamSonar();

    /**
     * @brief Initializes the sensor matrices and mathematical caches for the conical spread.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to configure the beam geometry and noise models.
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
    
    /** @brief Main acoustic processing loop. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Draws a debug cone in the Unreal Editor. */
    virtual void showRegion(float DeltaTime) override;

    /** @brief Determines if an Octree leaf falls inside the 3D conical acoustic beam. */
    virtual bool inRange(Octree* tree) override;
    
    /** @brief The total angular width of the conical acoustic beam. */
    UPROPERTY(EditAnywhere)
    float OpeningAngle = 30;

    /** @brief Number of bins used to slice the maximum range distance. */
    UPROPERTY(EditAnywhere)
    int32 RangeBins = 0;

    /** @brief Resolution of each range bin in cm. */
    UPROPERTY(EditAnywhere)
    float RangeRes = 0;

    /** @brief Number of bins used to divide the 360-degree rotation of the cone. */
    UPROPERTY(EditAnywhere)
    int32 CentralAngleBins = 0; 

    /** @brief Resolution of each central angle bin in degrees. */
    UPROPERTY(EditAnywhere)
    float CentralAngleRes = 0;

    /** @brief Number of bins used to divide the outward spread of the cone. */
    UPROPERTY(EditAnywhere)
    int32 OpeningAngleBins = 0; 

    /** @brief Resolution of each opening angle bin in degrees. */
    UPROPERTY(EditAnywhere)
    float OpeningAngleRes = 0;

private:
    /** @brief Pointer to the actor this sensor is attached to. */
    AActor* Parent;

    // Angles unique to the conical Singlebeam math
    float CentralAngle = 360;
    float minOpeningAngle;
    float maxOpeningAngle;
    float minCentralAngle;
    float maxCentralAngle;
    
    // Various mathematical computations cached for performance
    float sqrt3_2;
    float sinOffset;

    /** @brief Used to hold leaf counts when parallelized sorting/binning happens. */
    int32* count;
    
    // Noise generation profiles
    MultivariateNormal<1> addNoise;
    MultivariateNormal<1> multNoise;
    MultivariateUniform<1> rNoise;
};