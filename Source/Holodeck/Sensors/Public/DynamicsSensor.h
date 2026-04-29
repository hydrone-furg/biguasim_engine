// MIT License (c) 2021 BYU FRoStLab see LICENSE file
#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"
#include "DynamicsSensor.generated.h"

/**
 * @brief Ground Truth Dynamics Sensor.
 * Inherits from the HolodeckSensor class.
 * Reports a comprehensive array of physical states including linear acceleration, 
 * linear velocity, position, angular acceleration, angular velocity, and orientation. 
 * Ideal for implementing and validating custom dynamic control models.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UDynamicsSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UDynamicsSensor();

    /**
     * @brief Initializes the sensor and kinematic vectors.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to configure the data format (COM and Rotation type).
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /** * @brief Gets the number of items this sensor returns.
     * @return 18 items if UseRPY is true, or 19 items if returning Quaternions. 
     */
    int GetNumItems() override { return UseRPY ? 18 : 19; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(float); };
    
    /** @brief Main tick function for gathering physics data and calculating derivatives. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** @brief If true, queries physics data from the Center of Mass instead of the sensor's physical attachment location. */
    UPROPERTY(EditAnywhere)
    bool UseCOM = true;

    /** @brief If true, returns orientation as Roll-Pitch-Yaw (Euler angles). If false, returns a Quaternion (X, Y, Z, W). */
    UPROPERTY(EditAnywhere)
    bool UseRPY = true;

private:
    /** * @brief Pointer to whatever the sensor is attached to.
     * Used to extract the raw physics data. Not owned.
     */
    UPrimitiveComponent* Parent;

    // Kinematic state tracking for derivative calculations
    FVector LinearVelocityThen;
    FVector AngularVelocityThen;

    FVector LinearAcceleration;
    FVector LinearVelocity;
    FVector Position;
    FVector AngularAcceleration;
    FVector AngularVelocity;
    FRotator Rotation;
    FQuat Quat;
    FVector RPY;
};