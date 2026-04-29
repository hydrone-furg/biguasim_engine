#include "AnnotationComponent.h"
#include "Engine/StaticMeshActor.h"
#include "LandscapeProxy.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and applies the necessary post-processing materials for lens distortion and semantic segmentation.
 * Also attempts to load the existing palette.json file.
 */
UAnnotationComponent::UAnnotationComponent()
{
    SensorName = "AnnotationComponent";

    // AddPostProcessingMaterial(
    // TEXT("/Script/Engine.Material'/Game/PostProcessingMaterials/PhysicLensDistortion.PhysicLensDistortion'"));
    // AddPostProcessingMaterial(
    // TEXT("/Script/Engine.Material'/Game/PostProcessingMaterials/GTMaterial.GTMaterial'"));
    AddPostProcessingMaterial(
    TEXT("/Script/Engine.Material'/Game/PostProcessingMaterials/LensDistortion.LensDistortion'"));
    AddPostProcessingMaterial(
    TEXT("/Script/Engine.Material'/Game/PostProcessingMaterials/SegmentationMaterial.SegmentationMaterial'"));

    FilePath = FPaths::ProjectDir() + TEXT("../../palette.json");

    FString JsonString;

    if(!FFileHelper::LoadFileToString(JsonString, *FilePath) || JsonString.TrimStartAndEnd().IsEmpty()){
        JsonObject = MakeShared<FJsonObject>();
        JsonString = TEXT("{}");
    }

    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    FJsonSerializer::Deserialize(Reader, JsonObject);
}

/**
 * @brief Parses the JSON configuration sent by the Python client to dynamically set sensor parameters.
 * Handles the manual resizing of the render target and reallocation of shared memory if the resolution changes.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UAnnotationComponent::ParseSensorParms(FString ParmsJson)
{
    // Store old values
    int OldWidth = CaptureWidth;
    int OldHeight = CaptureHeight;

    Super::ParseSensorParms(ParmsJson);

    // --- FIX 2: MANUALLY READ JSON AND RESIZE ---
    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
        if (JsonParsed->HasTypedField<EJson::Number>("CaptureWidth")) {
            CaptureWidth = JsonParsed->GetIntegerField("CaptureWidth");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("CaptureHeight")) {
            CaptureHeight = JsonParsed->GetIntegerField("CaptureHeight");
        }
    }

    if (OldWidth != CaptureWidth || OldHeight != CaptureHeight) {
        
        UE_LOG(LogTemp, Warning, TEXT("AnnotationComponent Resizing: %d -> %d"), OldWidth, CaptureWidth);

        // 1. Resize Texture (GPU)
        if (TargetTexture) {
            TargetTexture->ResizeTarget(CaptureWidth, CaptureHeight);
        }

        // 2. Reallocate Memory (Shared Memory)
        if (Controller && Controller->GetServer()) {
             int TotalSize = CaptureWidth * CaptureHeight * sizeof(FColor);
             
             void* NewBuffer = Controller->GetServer()->Malloc(
                 UHolodeckServer::MakeKey(AgentName, SensorName + "_sensor_data"), 
                 TotalSize
             );
             
             // Update pointers
             this->Buffer = static_cast<FColor*>(NewBuffer);
             UHolodeckSensor::Buffer = NewBuffer; 
        }
    }
}

/**
 * @brief Initializes the sensor rendering parameters and begins tagging actors in the level.
 */
void UAnnotationComponent::InitializeSensor()
{
    if (TargetTexture) {
         TargetTexture->InitCustomFormat(CaptureWidth, CaptureHeight, PF_FloatRGBA, false);
    }

    Super::InitializeSensor();
    SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

    if (UWorld* world = GetWorld(); world)
    {
        TagActorsInLevel(*world, true);
    }
}

/**
 * @brief Sets the custom depth stencil value for a specific primitive component.
 * @param Component The component to modify.
 * @param IdColor The stencil ID value.
 * @param bShouldSetRenderCustomDepth Whether the component should render custom depth.
 */
void UAnnotationComponent::SetStencilValue(UPrimitiveComponent& Component, const uint8& IdColor, const bool bShouldSetRenderCustomDepth)
{
    Component.SetCustomDepthStencilValue(IdColor);
    Component.SetRenderCustomDepth(bShouldSetRenderCustomDepth && IdColor > 0);
}

/**
 * @brief Retrieves the maximum ID currently stored in the JSON object.
 * @return The next available maximum ID.
 */
int32 UAnnotationComponent::GetMaxJsonValue()
{
    if (!JsonObject.IsValid() || JsonObject->Values.Num() == 0)
    {
        return 1;
    }

    int32 MaxId = 1;

    for (const auto& Pair : JsonObject->Values)
    {
        const TSharedPtr<FJsonValue>& Value = Pair.Value;

        if (Value->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> SubObject = Value->AsObject();
            if (SubObject.IsValid() && SubObject->HasTypedField<EJson::Number>("id"))
            {
                int32 Id = static_cast<int32>(SubObject->GetIntegerField("id"));
                MaxId = FMath::Max(MaxId, Id);
            }
        }
    }

    return MaxId + 1;
}

/**
 * @brief Generates a unique FColor based on the ID and object unique ID.
 * @param ID The semantic ID.
 * @param id The unique identifier of the object.
 * @return A unique FColor representation.
 */
FColor UAnnotationComponent::GenerateUniqueColorFromID(int32 ID, uint32 id){
    FLinearColor Color(0.0f, 0.0f, 0.0f, 1.0f);
    Color.R = ID / 255.0f;
    Color.G = ((id & 0x00ff) >> 0) / 255.0f;
    Color.B = ((id & 0xff00) >> 8) / 255.0f;

    return Color.ToFColor(true);;
}

/**
 * @brief Evaluates an actor's name and applies semantic tags and stencil IDs.
 * Categorizes objects into predefined tags like Sky, Water, HolodeckAgent, or Landscape.
 * @param Actor The actor to tag.
 * @param bShouldTagForSemanticSegmentation Flag to apply custom depth rendering.
 */
void UAnnotationComponent::TagActor(AActor& Actor, bool bShouldTagForSemanticSegmentation){
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    Actor.GetComponents<UStaticMeshComponent>(StaticMeshComponents);
    FString BaseName;
    
    FString FullName = Actor.GetName();
    if (FullName.Split(TEXT("_"), &BaseName, nullptr))
    {
        // BaseName now holds "Sphere"
    }
    else
    {
        // No underscore, use full name
        BaseName = FullName;
    }


    if (Actor.Tags.IsEmpty())
    {   
        if(BaseName.Contains(TEXT("Sky"), ESearchCase::IgnoreCase) ||
            BaseName.Contains(TEXT("Weather"), ESearchCase::IgnoreCase) ||
            BaseName.Contains(TEXT("Fog"), ESearchCase::IgnoreCase) ||
            BaseName.Contains(TEXT("Cloud"), ESearchCase::IgnoreCase) ||
            BaseName.Contains(TEXT("Light"), ESearchCase::IgnoreCase) ||
            BaseName.Contains(TEXT("DefaultPhysicsVolume"), ESearchCase::IgnoreCase) ||
            BaseName.Contains(TEXT("ParticleEventManager"), ESearchCase::IgnoreCase) ||
            BaseName.Contains(TEXT("HUD"), ESearchCase::IgnoreCase))
            {
            Actor.Tags.Add(TEXT("Sky"));
        }else if(BaseName.Contains(TEXT("Water"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("Buoyancy"), ESearchCase::IgnoreCase)|| 
                BaseName.Contains(TEXT("WaterBodyCustom"), ESearchCase::IgnoreCase)){
            Actor.Tags.Add(TEXT("Water"));
        }else if(BaseName.Contains(TEXT("Holodeck"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("Agent"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("Game"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("Debug"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("Player"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("Controller"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("SpawnPropCommand"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("AbstractNavData"), ESearchCase::IgnoreCase) ){
            Actor.Tags.Add(TEXT("HolodeckAgent"));
        }else if(BaseName.Contains(TEXT("Landscape"), ESearchCase::IgnoreCase) ||
                BaseName.Contains(TEXT("RootComponent"), ESearchCase::IgnoreCase)){
            Actor.Tags.Add(TEXT("Landscape"));
        }
        else{
            Actor.Tags.Add(FName(*BaseName));
        }
    }

    FString Tag = Actor.Tags[0].ToString();

    if (Tag.Contains(TEXT("WaterBodyCustom"), ESearchCase::IgnoreCase)){
        Tag = TEXT("Water");
        Actor.Tags[0] = FName(*Tag);
    }

    if (!JsonObject->HasTypedField<EJson::Object>(Tag))
    {   

        TSharedPtr<FJsonObject> TagObject = MakeShared<FJsonObject>();
        uint8 Id = GetMaxJsonValue();
        FColor Color = GenerateUniqueColorFromID(Id, Actor.GetUniqueID());

        TagObject->SetNumberField("id", Id);


        TArray<TSharedPtr<FJsonValue>> ColorArray;

        TagObject->SetArrayField("color", ColorArray);

        JsonObject->SetObjectField(Tag, TagObject);
    }

    TArray<ULandscapeComponent*> Landscapes;
    Actor.GetComponents<ULandscapeComponent>(Landscapes);

    for (ULandscapeComponent* comp : Landscapes) {
            comp->SetCustomDepthStencilValue(JsonObject->GetObjectField(Tag)->GetIntegerField("id"));
            comp->SetRenderCustomDepth(true);
        }
    
    for (UStaticMeshComponent* Component : StaticMeshComponents){

        SetStencilValue(*Component, JsonObject->GetObjectField(Tag)->GetIntegerField("id"), bShouldTagForSemanticSegmentation); 
        if (!Component->IsVisible() || !Component->GetStaticMesh())
        {
            continue;
        }
    }
}
        
/**
 * @brief Explicitly sets the stencil ID for a landscape mesh.
 * @param mesh The landscape proxy actor.
 * @param object_id The ID to assign to the landscape components.
 */
void UAnnotationComponent::SetObjectStencilID(ALandscapeProxy* mesh, int object_id)
{
    if (object_id < 0) {
        mesh->bRenderCustomDepth = false;
    }
    else {
        mesh->CustomDepthStencilValue = object_id;
        mesh->bRenderCustomDepth = true;
    }

    for (ULandscapeComponent* comp : mesh->LandscapeComponents) {
        if (object_id < 0) {
            comp->SetRenderCustomDepth(false);
        }
        else {
            comp->SetCustomDepthStencilValue(object_id);
            comp->SetRenderCustomDepth(true);
        }
    }
}

/**
 * @brief Iterates over all actors in the world and tags them for semantic segmentation.
 * Also updates the colors from the external text file and saves the JSON palette.
 * @param World The current UWorld.
 * @param bShouldTagForSemanticSegmentation Flag to apply custom depth rendering.
 */
void UAnnotationComponent::TagActorsInLevel(UWorld& World, bool bShouldTagForSemanticSegmentation)
{
    for (TActorIterator<AActor> it(&World); it; ++it) {
        TagActor(**it, bShouldTagForSemanticSegmentation);
    }
    UpdateColorsFromTxt();
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

/**
 * @brief Executed every tick to retrieve pixels from the render target if the capture interval is met.
 */
void UAnnotationComponent::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

    TickCounter++;
    if (TickCounter >= TicksPerCapture) {
        RenderRequest.RetrievePixels(Buffer, TargetTexture, true);
        TickCounter -= TicksPerCapture;
    }
}

/**
 * @brief Reads the 'idColors.txt' file to map specific IDs to custom RGB values.
 * Parses the file and updates the corresponding color arrays within the JSON palette.
 */
void UAnnotationComponent::UpdateColorsFromTxt()
{
    FString TxtFilePath = FPaths::ProjectDir() + TEXT("../../idColors.txt"); 
    
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *TxtFilePath))
    {
        UE_LOG(LogHolodeck, Warning, TEXT("Nao foi possivel ler: %s"), *TxtFilePath);
        return;
    }

    TMap<int32, TArray<int32>> TempColorMap;

    for (const FString& Line : Lines)
    {
        if (Line.IsEmpty() || Line.StartsWith("#") || Line.StartsWith("//")) continue;

        // Format: "ID \t [R, G, B]"
        FString PartID, PartColors;
        
        if (Line.Split(TEXT("["), &PartID, &PartColors))
        {
            int32 Id = FCString::Atoi(*PartID);

            PartColors = PartColors.Replace(TEXT("]"), TEXT("")); 

            TArray<FString> RGBStrings;
            PartColors.ParseIntoArray(RGBStrings, TEXT(","), true);

            if (RGBStrings.Num() >= 3)
            {
                int32 R = FCString::Atoi(*RGBStrings[0].TrimStartAndEnd());
                int32 G = FCString::Atoi(*RGBStrings[1].TrimStartAndEnd());
                int32 B = FCString::Atoi(*RGBStrings[2].TrimStartAndEnd());

                TempColorMap.Add(Id, {R, G, B});
            }
        }
    }

    // 2. Update JSON
    if (JsonObject.IsValid())
    {
        for (auto& Pair : JsonObject->Values)
        {
            TSharedPtr<FJsonObject> ItemObj = Pair.Value->AsObject();
            if (ItemObj.IsValid() && ItemObj->HasTypedField<EJson::Number>("id"))
            {
                int32 JsonID = ItemObj->GetIntegerField("id");

                if (TempColorMap.Contains(JsonID))
                {
                    TArray<int32> Colors = TempColorMap[JsonID];

                    TArray<TSharedPtr<FJsonValue>> ColorArray;
                    ColorArray.Add(MakeShared<FJsonValueNumber>(Colors[0]));
                    ColorArray.Add(MakeShared<FJsonValueNumber>(Colors[1]));
                    ColorArray.Add(MakeShared<FJsonValueNumber>(Colors[2]));

                    ItemObj->SetArrayField("color", ColorArray);
                }
            }
        }
    }
}