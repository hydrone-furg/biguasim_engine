// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "OrientationSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
UOrientationSensor::UOrientationSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "OrientationSensor";
}

/**
 * @brief Initializes the sensor.
 * Extracts and caches the Holodeck Pawn Controller, the parent primitive component,
 * the root static mesh, and the current World reference for later use.
 */
void UOrientationSensor::InitializeSensor() {
    Super::InitializeSensor();

    Controller = static_cast<AHolodeckPawnController*>(this->GetAttachmentRootActor()->GetInstigator()->Controller);
    Parent = static_cast<UPrimitiveComponent*>(this->GetAttachParent());
    RootMesh = static_cast<UStaticMeshComponent*>(this->GetAttachParent());

    World = Parent->GetWorld();
}

/**
 * @brief Main execution loop that calculates the orientation matrix.
 * Retrieves the Forward, Right, and Up vectors of the sensor, negates the Right 
 * vector to get the Left vector, and populates a 9-element array (3x3 matrix) 
 * representing the absolute orientation in the client's coordinate system.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UOrientationSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    if (Parent != nullptr && RootMesh != nullptr) {
        FVector Forward = this->GetForwardVector();
        FVector Right = this->GetRightVector();
        FVector Up = this->GetUpVector();

        float* FloatBuffer = static_cast<float*>(Buffer);
        Forward = ConvertLinearVector(Forward, NoScale);
        FVector Left = ConvertLinearVector(-Right, NoScale); // Unreal is Right-Handed, Holodeck client is Left-Handed
        Up = ConvertLinearVector(Up, NoScale);

        // Populate a 3x3 matrix (Forward, Left, Up vectors)
        FloatBuffer[0] = Forward.X;
        FloatBuffer[3] = Forward.Y;
        FloatBuffer[6] = Forward.Z;
        
        FloatBuffer[1] = Left.X;
        FloatBuffer[4] = Left.Y;
        FloatBuffer[7] = Left.Z;
        
        FloatBuffer[2] = Up.X;
        FloatBuffer[5] = Up.Y;
        FloatBuffer[8] = Up.Z;
    }
}