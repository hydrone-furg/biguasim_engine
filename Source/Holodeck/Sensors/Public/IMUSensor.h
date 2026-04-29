// MIT License (c) 2019 BYU PCCL see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"
#include "MultivariateNormal.h"
#include "IMUSensor.generated.h"

/**
 * @brief Simulated Inertial Measurement Unit (IMU).
 * Inherits from the HolodeckSensor class.
 * Calculates and returns a 6-DOF data array representing local linear acceleration 
 * and angular velocity (gyroscope). Simulates real-world sensor imperfections including 
 * Gaussian noise and Random Walk Bias accumulation.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOLODECK_API UIMUSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UIMUSensor();

    /**
     * @brief Initializes the sensor and caches physics components.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Retrieves the current linear acceleration vector.
     * @return FVector representing local acceleration.
     */
    FVector GetAccelerationVector();

    /**
     * @brief Retrieves the current angular velocity vector.
     * @return FVector representing local angular velocity.
     */
    FVector GetAngularVelocityVector();

    /**
     * @brief Parses JSON parameters to set noise limits and bias configurations.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** * @brief Gets the number of items this sensor returns.
     * @return 12 items if ReturnBias is true (6 IMU data + 6 Bias data), otherwise 6 items.
     */
    int GetNumItems() override { return ReturnBias ? 12 : 6; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function calculating kinematics and applying noise models. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief Determines if the sensor should export its internal accumulated bias state to the buffer. */
    UPROPERTY(EditAnywhere)
    bool ReturnBias = false;

private:
    /**
     * @brief Internal function to calculate the linear acceleration vector.
     * @param DeltaTime Time elapsed since the last tick.
     */
    void CalculateAccelerationVector(float DeltaTime);

    /**
     * @brief Internal function to calculate the angular velocity vector.
     */
    void CalculateAngularVelocityVector();

    /** @brief Pointer to the parent component for physics querying. */
    UPrimitiveComponent* Parent;

    UWorld* World;
    AWorldSettings* WorldSettings;
    float WorldGravity;

    FVector VelocityThen;
    FVector VelocityNow;
    FRotator RotationNow;

    FVector LinearAccelerationVector;
    FVector AngularVelocityVector;

    // Used for simulating sensor noise and drift
    MultivariateNormal<3> mvnAccel;
    MultivariateNormal<3> mvnOmega;
    MultivariateNormal<3> mvnBiasAccel;
    MultivariateNormal<3> mvnBiasOmega;
    
    FVector BiasAccel = FVector(0);
    FVector BiasOmega = FVector(0);
};