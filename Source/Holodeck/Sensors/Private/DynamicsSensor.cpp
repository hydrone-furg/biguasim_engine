// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "DynamicsSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
UDynamicsSensor::UDynamicsSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "DynamicsSensor";
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Configures whether to track the Center of Mass (UseCOM) and whether to output 
 * orientation as Roll-Pitch-Yaw or Quaternions (UseRPY).
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UDynamicsSensor::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

        if (JsonParsed->HasTypedField<EJson::Boolean>("UseCOM")) {
            UseCOM = JsonParsed->GetBoolField("UseCOM");
        }
        if (JsonParsed->HasTypedField<EJson::Boolean>("UseRPY")) {
            UseRPY = JsonParsed->GetBoolField("UseRPY");
        }

    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("UDynamicsSensor::ParseSensorParms:: Unable to parse json."));
    }
}

/**
 * @brief Initializes the sensor and resets all kinematic tracking vectors.
 */
void UDynamicsSensor::InitializeSensor() {
    Super::InitializeSensor();

    // You need to get the pointer to the object you are attached to. 
    Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());

    // Initialize all vectors
    LinearVelocityThen = FVector();
    AngularVelocityThen = FVector();

    LinearAcceleration = FVector();
    LinearVelocity = FVector();
    Position = FVector();
    AngularAcceleration = FVector();
    AngularVelocity = FVector();
    Rotation = FRotator();
}

/**
 * @brief Main execution loop that calculates and reports full 6-DOF kinematics.
 * Computes linear and angular derivatives over time (DeltaTime) to find accelerations.
 * Outputs an 18-element (if RPY) or 19-element (if Quaternion) array to the shared memory.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UDynamicsSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    // Check if your parent pointer is valid, and if the sensor is on. 
    if (Parent != nullptr && bOn) {
        // Linear Acceleration & Velocity
        LinearVelocityThen = LinearVelocity;
        if(UseCOM){
            LinearVelocity = Parent->GetPhysicsLinearVelocity();
        }
        else{
            LinearVelocity  = Parent->GetPhysicsLinearVelocityAtPoint(this->GetComponentLocation());
        }
        LinearVelocity = ConvertLinearVector(LinearVelocity, UEToClient);
        LinearAcceleration = (LinearVelocity - LinearVelocityThen) / DeltaTime;

        // Position
        if(UseCOM){
            Position = this->GetAttachParent()->GetComponentLocation();
        }
        else{
            Position = this->GetComponentLocation();
        }
        Position = ConvertLinearVector(Position, UEToClient);
        
        // Ang Acceleration & Velocity
        AngularVelocityThen = AngularVelocity;
        // Angular velocity doesn't depend on the location on the object
        AngularVelocity = Parent->GetPhysicsAngularVelocityInRadians();
        AngularVelocity = ConvertAngularVector(AngularVelocity, NoScale);
        AngularAcceleration = (AngularVelocity - AngularVelocityThen) / DeltaTime;

        // Rotation
        if(UseCOM){
            Rotation = this->GetAttachParent()->GetComponentRotation();
        }
        else{
            Rotation = this->GetComponentRotation();
        }

        // Fill everything in
        float* FloatBuffer = static_cast<float*>(Buffer);
        FloatBuffer[0] = LinearAcceleration.X;
        FloatBuffer[1] = LinearAcceleration.Y;
        FloatBuffer[2] = LinearAcceleration.Z;
        
        FloatBuffer[3] = LinearVelocity.X;
        FloatBuffer[4] = LinearVelocity.Y;
        FloatBuffer[5] = LinearVelocity.Z;
        
        FloatBuffer[6] = Position.X;
        FloatBuffer[7] = Position.Y;
        FloatBuffer[8] = Position.Z;
        
        FloatBuffer[9] = AngularAcceleration.X;
        FloatBuffer[10] = AngularAcceleration.Y;
        FloatBuffer[11] = AngularAcceleration.Z;
        
        FloatBuffer[12] = AngularVelocity.X;
        FloatBuffer[13] = AngularVelocity.Y;
        FloatBuffer[14] = AngularVelocity.Z;
        
        if(UseRPY){
            RPY = RotatorToRPY(Rotation);
            FloatBuffer[15] = RPY.X;
            FloatBuffer[16] = RPY.Y;
            FloatBuffer[17] = RPY.Z;
        }
        else{
            Quat = Rotation.Quaternion();
            // Flip quaternion y axis to match client coordinate system
            FloatBuffer[15] = Quat.X;
            FloatBuffer[16] = -1 * Quat.Y;
            FloatBuffer[17] = Quat.Z;
            FloatBuffer[18] = -1 * Quat.W;
        }
    }
}