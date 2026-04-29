#include "RGBDCamera.h"
#include "Engine.h"

#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "ShowFlags.h"

#include "Materials/Material.h"
#include "RHICommandList.h"
#include "Modules/ModuleManager.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name to RGBDCamera.
 */
URGBDCamera::URGBDCamera()
{
    SensorName = "RGBDCamera";
    Parent = nullptr;
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Inherits default camera parsing behavior from UCameraSensor.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void URGBDCamera::ParseSensorParms(FString ParmsJson)
{
    Super::ParseSensorParms(ParmsJson);
}

/**
 * @brief Initializes the rendering pipeline for the RGB-D Camera.
 * Configures the SceneCaptureComponent to simultaneously capture Scene Color (RGB) 
 * and Scene Depth (Z-Buffer) in a single render pass to optimize performance.
 */
void URGBDCamera::InitializeSensor()
{
    Super::InitializeSensor();
    Parent = this->GetAttachmentRootActor();
    
    // The magic flag: Captures Color and Depth in the same texture target
    SceneCapture->CaptureSource = SCS_SceneColorSceneDepth;
}

/**
 * @brief Main execution loop that captures the combined RGB-D frame.
 * Checks the tick counter and, when the capture interval is met, queues a render 
 * request to retrieve the combined pixels into the shared memory buffer.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void URGBDCamera::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

    TickCounter++;
    if (TickCounter >= TicksPerCapture) {
        RenderRequest.RetrievePixels(Buffer, TargetTexture);

        TickCounter -= TicksPerCapture;
    }
}