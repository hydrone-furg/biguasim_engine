#include "DepthCamera.h"
#include "Engine.h"
#include "Engine/PostProcessVolume.h" // Necessário para identificar volumes
#include "EngineUtils.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"

/**
 * @brief Default Constructor.
 * Sets the sensor identifier to DepthCamera.
 */
UDepthCamera::UDepthCamera() {
    SensorName = "DepthCamera";
}

/**
 * @brief Gets the size of a single data item returned by the sensor.
 * @return 16 bytes (4 floats of 32 bits) per pixel, maintaining the (h, w, 4) array shape.
 */
int UDepthCamera::GetItemSize() {
    // Returns 16 bytes (4 floats of 32 bits) per pixel
    // AirSim reads Float16, but we convert to Float32 at the end
    return sizeof(float) * 4; 
}

/**
 * @brief Gets the total number of items (pixels) in the buffer.
 * @return Total pixel count (CaptureWidth * CaptureHeight).
 */
int UDepthCamera::GetNumItems() {
    return CaptureWidth * CaptureHeight;
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Sets the capture resolution and the tick rate.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UDepthCamera::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);
    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
        if (JsonParsed->HasTypedField<EJson::Number>("TicksPerCapture")) 
            TicksPerCapture = JsonParsed->GetIntegerField("TicksPerCapture");
        if (JsonParsed->HasTypedField<EJson::Number>("CaptureWidth")) 
            CaptureWidth = JsonParsed->GetIntegerField("CaptureWidth");
        if (JsonParsed->HasTypedField<EJson::Number>("CaptureHeight")) 
            CaptureHeight = JsonParsed->GetIntegerField("CaptureHeight");
    }
}

/**
 * @brief Initializes the rendering pipeline for the Depth Camera.
 * Configures the SceneCaptureComponent to read raw SceneDepth instead of PostProcess HDR.
 * Disables translucency and atmospheric effects to ensure the camera rays ignore 
 * custom water assets and strictly measure solid geometry distances.
 */
void UDepthCamera::InitializeSensor() {
    Super::InitializeSensor();

    // Configure for FloatRGBA (High precision)
    TargetTexture->InitCustomFormat(CaptureWidth, CaptureHeight, PF_FloatRGBA, true);
    TargetTexture->TargetGamma = 1.0f; 
    TargetTexture->UpdateResource();
    
    // Read raw geometry depth, bypassing any PostProcess or water rendering
    SceneCapture->CaptureSource = SCS_SceneDepth; 

    // Hide the agent itself so it doesn't block the camera rays
    SceneCapture->HiddenActors.Add(GetOwner());
    TArray<AActor*> Attached;
    GetOwner()->GetAttachedActors(Attached);
    SceneCapture->HiddenActors.Append(Attached);

    // Disable cosmetic rendering flags to capture pure geometry
    SceneCapture->ShowFlags.SetTranslucency(false); // Ignores water surfaces
    SceneCapture->ShowFlags.SetFog(false);
    SceneCapture->ShowFlags.SetAtmosphere(false);
    SceneCapture->ShowFlags.SetLighting(false);
    
    // Clears any leftover PostProcess materials
    SceneCapture->PostProcessSettings.WeightedBlendables.Array.Empty();

    // Push the clipping plane to 20cm to ignore geometry directly glued to the lens
    SceneCapture->CustomNearClippingPlane = 20.0f; 

    FloatBuffer.SetNumUninitialized(CaptureWidth * CaptureHeight);
}

/**
 * @brief Main execution loop that captures and formats the depth buffer.
 * Queues a rendering command to extract the raw FFloat16Color depth buffer,
 * then parses the Red channel into a 32-bit float array for the shared memory.
 */
void UDepthCamera::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    TickCounter++;
    if (TickCounter >= TicksPerCapture) {
        
        FTextureRenderTargetResource* RenderResource = TargetTexture->GameThread_GetRenderTargetResource();
        
        if (RenderResource) {
            
            struct FReadSurfaceContext {
                FTextureRenderTargetResource* SrcRenderTarget;
                TArray<FFloat16Color>* OutData; 
                FIntRect Rect;
            };

            FReadSurfaceContext ReadContext = {
                RenderResource,
                &FloatBuffer,
                FIntRect(0, 0, CaptureWidth, CaptureHeight)
            };

            // RENDER COMMAND ENQUEUE
            ENQUEUE_RENDER_COMMAND(ReadDepthCommand)(
                [ReadContext](FRHICommandListImmediate& RHICmdList) {
                    FTexture2DRHIRef Texture2D = ReadContext.SrcRenderTarget->GetRenderTargetTexture();
                    if (Texture2D.IsValid()) {
                        // Reads the data as real HDR Floats (bypassing sRGB compression)
                        RHICmdList.ReadSurfaceFloatData(
                            Texture2D,
                            ReadContext.Rect,
                            *ReadContext.OutData,
                            CubeFace_PosX,
                            0,
                            0
                        );
                    }
                }
            );

            FlushRenderingCommands();

            // FINAL CONVERSION
            if (FloatBuffer.Num() > 0) {
                // Buffer is the Holodeck shared memory pointer
                float* DestPtr = (float*)Buffer; 
                
                for (const FFloat16Color& Item : FloatBuffer) {
                    // We write 4 floats to maintain the self.shape=(h,w,4) array structure
                    
                    // 1. Channel R (Our precious Depth)
                    *DestPtr++ = Item.R.GetFloat();
                    
                    // 2. Channel G (Empty)
                    *DestPtr++ = 0.0f;
                    
                    // 3. Channel B (Empty)
                    *DestPtr++ = 0.0f;
                    
                    // 4. Channel A (Alpha/Opacity)
                    *DestPtr++ = 1.0f;
                }
            }
        }
        TickCounter -= TicksPerCapture;
    }
}