// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "LocationSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
ULocationSensor::ULocationSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "LocationSensor";
}

/**
 * @brief Parses the JSON configuration sent by the Python client to dynamically set sensor parameters.
 * Extracts the multivariate noise matrices (Sigma and Covariance) to optionally simulate 
 * inaccuracy in the ground truth tracking.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void ULocationSensor::ParseSensorParms(FString ParmsJson) {
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

    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("ULocationSensor::ParseSensorParms:: Unable to parse json."));
    }
}

/**
 * @brief Initializes the sensor and fetches the attached parent component.
 */
void ULocationSensor::InitializeSensor() {
    Super::InitializeSensor();

    // You need to get the pointer to the object you are attached to. 
    Parent = this->GetAttachParent();
}

/**
 * @brief Main execution loop that calculates and reports the absolute global position.
 * Retrieves the raw location from the Unreal Engine world, converts it to the client's 
 * coordinate system, applies multivariate normal noise, and writes to the shared memory buffer.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void ULocationSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    // Check if your parent pointer is valid, and if the sensor is on. 
    // Then get the location and buffer, then send the location to the buffer. 
    if (Parent != nullptr && bOn) {
        FVector Location = this->GetComponentLocation();
        float* FloatBuffer = static_cast<float*>(Buffer);
        
        Location = ConvertLinearVector(Location, UEToClient);
        Location += mvn.sampleFVector();
        
        FloatBuffer[0] = Location.X;
        FloatBuffer[1] = Location.Y;
        FloatBuffer[2] = Location.Z;
    }
}