#include "Holodeck.h"
#include "Conversion.h"
#include "RotationSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
URotationSensor::URotationSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "RotationSensor";
}

/**
 * @brief Initializes the sensor.
 * Fetches and caches the root actor that this sensor is attached to.
 */
void URotationSensor::InitializeSensor() {
    Super::InitializeSensor();

    // You need to get the pointer to the object the sensor is attached to. 
    Parent = this->GetAttachmentRootActor();
}

/**
 * @brief Main execution loop that calculates and reports the Euler rotation.
 * Retrieves the raw rotation from the Unreal Engine component, converts it into 
 * Roll, Pitch, and Yaw (RPY) format, and writes the three values to the shared memory buffer.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void URotationSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    if (Parent != nullptr && bOn) {
        FRotator Rotation = this->GetComponentRotation();
        
        // Convert Unreal Engine's FRotator to a standardized Roll-Pitch-Yaw vector
        FVector EulerAngles = RotatorToRPY(Rotation);
        
        float* FloatBuffer = static_cast<float*>(Buffer);
        FloatBuffer[0] = EulerAngles.X; // Roll
        FloatBuffer[1] = EulerAngles.Y; // Pitch
        FloatBuffer[2] = EulerAngles.Z; // Yaw
    }
}