// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "PoseSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
UPoseSensor::UPoseSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "PoseSensor";
}

/**
 * @brief Initializes the sensor.
 * Extracts and caches the Holodeck Pawn Controller, the parent primitive component,
 * the root static mesh, and the current World reference for later use.
 */
void UPoseSensor::InitializeSensor() {
    Super::InitializeSensor();

    Controller = static_cast<AHolodeckPawnController*>(this->GetAttachmentRootActor()->GetInstigator()->Controller);
    Parent = static_cast<UPrimitiveComponent*>(this->GetAttachParent());
    RootMesh = static_cast<UStaticMeshComponent*>(this->GetAttachParent());

    World = Parent->GetWorld();
}

/**
 * @brief Main execution loop that calculates the 4x4 Homogeneous Transformation Matrix.
 * Retrieves the Forward, Right, Up vectors, and the Location of the sensor.
 * Negates the Right vector to convert from Unreal's Right-Handed to the client's Left-Handed 
 * coordinate system, and packs everything into a 16-element array.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UPoseSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    if (Parent != nullptr && RootMesh != nullptr) {
        FVector Forward = this->GetForwardVector();
        FVector Right = this->GetRightVector();
        FVector Up = this->GetUpVector();
        FVector Location = this->GetComponentLocation();

        float* FloatBuffer = static_cast<float*>(Buffer);
        Forward = ConvertLinearVector(Forward, NoScale);
        FVector Left = ConvertLinearVector(-Right, NoScale);
        Up = ConvertLinearVector(Up, NoScale);
        Location = ConvertLinearVector(Location, UEToClient);

        // Insert Rotation Matrix (3x3 top-left)
        FloatBuffer[0] = Forward.X;
        FloatBuffer[4] = Forward.Y;
        FloatBuffer[8] = Forward.Z;
        
        FloatBuffer[1] = Left.X;
        FloatBuffer[5] = Left.Y;
        FloatBuffer[9] = Left.Z;
        
        FloatBuffer[2] = Up.X;
        FloatBuffer[6] = Up.Y;
        FloatBuffer[10] = Up.Z;

        // Insert Position (3x1 top-right column)
        FloatBuffer[3] = Location.X;
        FloatBuffer[7] = Location.Y;
        FloatBuffer[11] = Location.Z;

        // Insert Bottom Row (1x4)
        FloatBuffer[12] = 0;
        FloatBuffer[13] = 0;
        FloatBuffer[14] = 0;
        FloatBuffer[15] = 1;
    }
}