// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "MagnetometerSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
UMagnetometerSensor::UMagnetometerSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "MagnetometerSensor";
}

/**
 * @brief Parses the JSON configuration sent by the Python client to dynamically set sensor parameters.
 * Extracts the multivariate noise matrices (Sigma and Covariance). Also allows overriding the 
 * default global magnetic vector, handling the conversion from a Right-Handed coordinate system 
 * (client) to Unreal Engine's Left-Handed system by flipping the Y-axis.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UMagnetometerSensor::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

        if (JsonParsed->HasTypedField<EJson::Number>("Sigma")) {
            mvn.initSigma(JsonParsed->GetNumberField("Sigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("Sigma")) {
            mvn.initSigma(JsonParsed->GetArrayField("Sigma"));
        }

        if (JsonParsed->HasTypedField<EJson::Number>("Cov")) {
            mvn.initCov(JsonParsed->GetNumberField("Cov"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("Cov")) {
            mvn.initCov(JsonParsed->GetArrayField("Cov"));
        }

        if (JsonParsed->HasTypedField<EJson::Array>("MagneticVector")) {
            TArray<TSharedPtr<FJsonValue>> b = JsonParsed->GetArrayField("MagneticVector");

            if(b.Num() == 3){
                MeasuredVector[0] = b[0]->AsNumber();
                MeasuredVector[1] = -1 * b[1]->AsNumber(); // Switch to Left-Handed for UE4
                MeasuredVector[2] = b[2]->AsNumber();
            }
            else{
                UE_LOG(LogHolodeck, Fatal, TEXT("UMagnetometerSensor::ParseSensorParms:: MagneticVector had wrong size."));
            }
        }
    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("UMagnetometerSensor::ParseSensorParms:: Unable to parse json."));
    }
}

/**
 * @brief Initializes the sensor and fetches the attached parent component.
 */
void UMagnetometerSensor::InitializeSensor() {
    Super::InitializeSensor();

    // You need to get the pointer to the object you are attached to. 
    Parent = this->GetAttachParent();
}

/**
 * @brief Main execution loop that calculates and reports the magnetic field.
 * Unrotates the global magnetic vector into the sensor's local coordinate frame, 
 * applies Gaussian noise, and converts the resulting vector back to a Right-Handed 
 * coordinate system before writing to the shared memory buffer.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UMagnetometerSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    // Check if your parent pointer is valid, and if the sensor is on. 
    if (Parent != nullptr && bOn) {
        float* FloatBuffer = static_cast<float*>(Buffer);

        FRotator Rotation = this->GetComponentRotation();
        
        // Transform the global vector into the sensor's local frame
        FVector measurement = Rotation.UnrotateVector(MeasuredVector);
        measurement += mvn.sampleFVector();

        FloatBuffer[0] = measurement.X;
        FloatBuffer[1] = -1 * measurement.Y; // Switch back to Right-Handed for the client
        FloatBuffer[2] = measurement.Z;
    }
}