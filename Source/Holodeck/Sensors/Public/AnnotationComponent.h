#pragma once

#include "CameraSensor.h"
#include "Holodeck.h"
#include "ShaderBasedSensor.h"
#include "Tagger.h"
#include "LandscapeProxy.h"
#include "AnnotationComponent.generated.h"

/**
 * @brief Semantic segmentation camera component.
 * Inherits from UShaderBasedSensor.
 * Automatically tags actors in the world and renders them with specific colors based on 
 * their tags or custom ID mappings loaded from an external text file.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UAnnotationComponent : public UShaderBasedSensor
{
    GENERATED_BODY()

public:

    /**
     * @brief Default Constructor.
     */
    UAnnotationComponent();

    /**
     * @brief Initializes the sensor rendering parameters and begins tagging actors in the level.
     */
    virtual void InitializeSensor() override;

    /**
     * @brief Parses JSON parameters to set resolution, handling memory reallocation if needed.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

protected:
    /**
     * @brief Reads the 'idColors.txt' file to map specific IDs to custom RGB values.
     */
    void UpdateColorsFromTxt();

    /** @brief Gets the total number of pixels in the capture frame. */
    virtual int GetNumItems() override { return CaptureWidth * CaptureHeight; };
    
    /** @brief Gets the size of each pixel data structure (FColor). */
    virtual int GetItemSize() override { return sizeof(FColor); };

    /**
     * @brief Executes the render extraction command and writes to shared memory.
     */
    virtual void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /**
     * @brief Sets the custom depth stencil value for a specific primitive component.
     */
    void SetStencilValue(UPrimitiveComponent& Component, const uint8& IdColor, const bool bShouldSetRenderCustomDepth);

    /**
     * @brief Retrieves the maximum ID currently stored in the JSON object.
     */
    int32 GetMaxJsonValue();
    
    /**
     * @brief Generates a unique FColor based on the ID and object unique ID.
     */
    FColor GenerateUniqueColorFromID(int32 ID, uint32 id);
    
    /**
     * @brief Evaluates an actor's name and applies semantic tags and stencil IDs.
     */
    void TagActor(AActor& Actor, bool bShouldTagForSemanticSegmentation);

    /**
     * @brief Iterates over all actors in the world and tags them for semantic segmentation.
     */
    void TagActorsInLevel(UWorld& World, bool bShouldTagForSemanticSegmentation);
    
    /**
     * @brief Explicitly sets the stencil ID for a landscape mesh.
     */
    void SetObjectStencilID(ALandscapeProxy* mesh, int object_id);

private:
    /** @brief Internal counter to manage TicksPerCapture logic. */
    int TickCounter = 0;

    /** @brief File path for the palette.json file. */
    FString FilePath;
    
    /** @brief Shared pointer to the JSON object storing tag and color mappings. */
    TSharedPtr<FJsonObject> JsonObject;
    
    /** @brief String buffer used for writing the JSON output. */
    FString OutputString;
    
    /** @brief Shared reference to the JSON writer object. */
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
};