// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "VelocitySensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
UVelocitySensor::UVelocitySensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "VelocitySensor";
}

/**
 * @brief Initializes the sensor.
 * Fetches and caches the root primitive component that this sensor is attached to.
 */
void UVelocitySensor::InitializeSensor() {
    Super::InitializeSensor();

    // You need to get the pointer to the object the sensor is attached to. 
    Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());
}

/**
 * @brief Main execution loop that calculates and reports the linear velocity.
 * Retrieves the true physical linear velocity of the attached component at its specific 
 * location, converts it to the client's coordinate system, and writes the XYZ vector to the buffer.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UVelocitySensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    // Check if your parent pointer is valid, and if the sensor is on. 
    if (Parent != nullptr && bOn) {
        FVector Velocity = Parent->GetPhysicsLinearVelocityAtPoint(this->GetComponentLocation());
        Velocity = ConvertLinearVector(Velocity, UEToClient);
        
        float* FloatBuffer = static_cast<float*>(Buffer);
        FloatBuffer[0] = Velocity.X;
        FloatBuffer[1] = Velocity.Y;
        FloatBuffer[2] = Velocity.Z;
    }
}