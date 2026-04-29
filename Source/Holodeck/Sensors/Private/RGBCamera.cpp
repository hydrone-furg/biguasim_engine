// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "RGBCamera.h"
#include "Json.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name to RGBCamera.
 */
URGBCamera::URGBCamera() {
    SensorName = "RGBCamera";
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Extracts the TicksPerCapture parameter to dynamically control the camera's framerate.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void URGBCamera::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
        
        if (JsonParsed->HasTypedField<EJson::Number>("TicksPerCapture")) {
            TicksPerCapture = JsonParsed->GetIntegerField("TicksPerCapture");
        }
    } else {
        UE_LOG(LogHolodeck, Fatal, TEXT("URGBCamera::ParseSensorParms:: Unable to parse json."));
    } 
}

/**
 * @brief Initializes the rendering pipeline for the RGB Camera.
 * Sets the SceneCaptureComponent to output standard Low Dynamic Range (LDR) final color,
 * which includes lighting, shadows, and basic post-processing.
 */
void URGBCamera::InitializeSensor() {
    Super::InitializeSensor();

    // Set up everything for the scenecapturecomponent2d
    SceneCapture->CaptureSource = SCS_FinalColorLDR; // Pick what type of output you want to be sent to the texture target.
}

/**
 * @brief Main execution loop that captures the camera frame.
 * Waits for the specified number of ticks, then queues a render request to copy 
 * the texture target pixels into the shared memory buffer.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void URGBCamera::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

    TickCounter++;
    if (TickCounter == TicksPerCapture) {
        RenderRequest.RetrievePixels(Buffer, TargetTexture);
        TickCounter = 0;
    }
}