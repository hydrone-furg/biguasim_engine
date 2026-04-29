// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "IMUSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
UIMUSensor::UIMUSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "IMUSensor";
}

/**
 * @brief Parses the JSON configuration sent by the Python client to dynamically set sensor parameters.
 * Extracts the noise matrices (Sigma and Covariance) for Acceleration and Angular Velocity.
 * Also parses the Random Walk Bias parameters for both, allowing realistic IMU drift simulation.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UIMUSensor::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

        if (JsonParsed->HasTypedField<EJson::Boolean>("ReturnBias")) {
            ReturnBias = JsonParsed->GetBoolField("ReturnBias");
        }

        // Acceleration noise
        if (JsonParsed->HasTypedField<EJson::Number>("AccelSigma")) {
            mvnAccel.initSigma(JsonParsed->GetNumberField("AccelSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AccelSigma")) {
            mvnAccel.initSigma(JsonParsed->GetArrayField("AccelSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AccelCov")) {
            mvnAccel.initCov(JsonParsed->GetNumberField("AccelCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AccelCov")) {
            mvnAccel.initCov(JsonParsed->GetArrayField("AccelCov"));
        }

        // Angular Velocity noise
        if (JsonParsed->HasTypedField<EJson::Number>("AngVelSigma")) {
            mvnOmega.initSigma(JsonParsed->GetNumberField("AngVelSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AngVelSigma")) {
            mvnOmega.initSigma(JsonParsed->GetArrayField("AngVelSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AngVelCov")) {
            mvnOmega.initCov(JsonParsed->GetNumberField("AngVelCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AngVelCov")) {
            mvnOmega.initCov(JsonParsed->GetArrayField("AngVelCov"));
        }

        // Acceleration Bias noise (Random Walk)
        if (JsonParsed->HasTypedField<EJson::Number>("AccelBiasSigma")) {
            mvnBiasAccel.initSigma(JsonParsed->GetNumberField("AccelBiasSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AccelBiasSigma")) {
            mvnBiasAccel.initSigma(JsonParsed->GetArrayField("AccelBiasSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AccelBiasCov")) {
            mvnBiasAccel.initCov(JsonParsed->GetNumberField("AccelBiasCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AccelBiasCov")) {
            mvnBiasAccel.initCov(JsonParsed->GetArrayField("AccelBiasCov"));
        }

        // Angular Velocity Bias noise (Random Walk)
        if (JsonParsed->HasTypedField<EJson::Number>("AngVelBiasSigma")) {
            mvnBiasOmega.initSigma(JsonParsed->GetNumberField("AngVelBiasSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AngVelBiasSigma")) {
            mvnBiasOmega.initSigma(JsonParsed->GetArrayField("AngVelBiasSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AngVelBiasCov")) {
            mvnBiasOmega.initCov(JsonParsed->GetNumberField("AngVelBiasCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("AngVelBiasCov")) {
            mvnBiasOmega.initCov(JsonParsed->GetArrayField("AngVelBiasCov"));
        }

    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("UIMUSensor::ParseSensorParms:: Unable to parse json."));
    }
}

/**
 * @brief Initializes the sensor, caches the parent component, and extracts world gravity.
 */
void UIMUSensor::InitializeSensor() {
    Super::InitializeSensor();

    // Cache important variables
    Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());

    World = Parent->GetWorld();
    WorldSettings = World->GetWorldSettings(false, false);
    WorldGravity = WorldSettings->GetGravityZ();

    VelocityThen = FVector();
    VelocityNow = FVector();
    LinearAccelerationVector = FVector();
    AngularVelocityVector = FVector();
}

/**
 * @brief Main execution loop that calculates and reports IMU data.
 * Computes the 6-DOF inertial data, applies coordinate transformations, 
 * introduces Gaussian noise and random walk bias, and writes to shared memory.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UIMUSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) {
    if (Parent != nullptr && bOn) {
        CalculateAccelerationVector(DeltaTime);
        CalculateAngularVelocityVector();

        float* FloatBuffer = static_cast<float*>(Buffer);

        // Convert before sending to user side.
        LinearAccelerationVector = ConvertLinearVector(LinearAccelerationVector, UEToClient);
        AngularVelocityVector = ConvertAngularVector(AngularVelocityVector, NoScale);

        // Introduce noise and accumulating bias (Random Walk)
        BiasAccel += mvnBiasAccel.sampleFVector();
        BiasOmega += mvnBiasOmega.sampleFVector();
        LinearAccelerationVector += BiasAccel + mvnAccel.sampleFVector();
        AngularVelocityVector    += BiasOmega + mvnOmega.sampleFVector();

        // Write primary IMU data
        FloatBuffer[0] = LinearAccelerationVector.X;
        FloatBuffer[1] = LinearAccelerationVector.Y;
        FloatBuffer[2] = LinearAccelerationVector.Z;
        FloatBuffer[3] = AngularVelocityVector.X;
        FloatBuffer[4] = AngularVelocityVector.Y;
        FloatBuffer[5] = AngularVelocityVector.Z;

        // Optionally write current bias state for debugging/filtering
        if(ReturnBias){
            FloatBuffer[6] = BiasAccel.X;
            FloatBuffer[7] = BiasAccel.Y;
            FloatBuffer[8] = BiasAccel.Z;
            FloatBuffer[9] = BiasOmega.X;
            FloatBuffer[10] = BiasOmega.Y;
            FloatBuffer[11] = BiasOmega.Z;
        }
    }
}

/**
 * @brief Calculates the linear acceleration vector of the sensor.
 * Uses the derivative of velocity over time, compensating for world gravity,
 * and transforms the resulting vector into the sensor's local coordinate frame.
 * @param DeltaTime The time elapsed since the last tick.
 */
void UIMUSensor::CalculateAccelerationVector(float DeltaTime) {
    VelocityThen = VelocityNow;
    VelocityNow  = Parent->GetPhysicsLinearVelocityAtPoint(this->GetComponentLocation());

    RotationNow = this->GetComponentRotation();

    LinearAccelerationVector = VelocityNow - VelocityThen;
    LinearAccelerationVector /= DeltaTime;

    LinearAccelerationVector += FVector(0.0, 0.0, -WorldGravity);

    LinearAccelerationVector = RotationNow.UnrotateVector(LinearAccelerationVector); // changes world axis to local axis
}

/**
 * @brief Calculates the angular velocity (gyroscope) vector of the sensor.
 * Reads the physics angular velocity and transforms it from world angles to local angles.
 */
void UIMUSensor::CalculateAngularVelocityVector() {
    AngularVelocityVector = Parent->GetPhysicsAngularVelocityInRadians();

    // Explicit self-assignment to guarantee struct integrity (Unreal Engine quirk)
    AngularVelocityVector.X = AngularVelocityVector.X;
    AngularVelocityVector.Y = AngularVelocityVector.Y;
    AngularVelocityVector.Z = AngularVelocityVector.Z;

    AngularVelocityVector = RotationNow.UnrotateVector(AngularVelocityVector); // Rotate from world angles to local angles.
}

/**
 * @brief Retrieves the current local linear acceleration vector.
 * @return The acceleration vector (X, Y, Z).
 */
FVector UIMUSensor::GetAccelerationVector() {
    return LinearAccelerationVector;
}

/**
 * @brief Retrieves the current local angular velocity vector.
 * @return The angular velocity vector (Roll, Pitch, Yaw).
 */
FVector UIMUSensor::GetAngularVelocityVector() {
    return AngularVelocityVector;
}