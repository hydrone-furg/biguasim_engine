#include "Holodeck.h"
#include "RenderRequest.h"
#include "AnnotationComponent.h"

#include <cstdint>

DEFINE_STAT(STAT_CameraExecuteTask);
DEFINE_STAT(STAT_CameraExecuteTask_ReadSurfaceData);
DEFINE_STAT(STAT_CameraExecuteTask_Memcpy);

void LogColors(const uint8_t* Colors, int32 Length)
{
    if (!Colors || Length <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Colors pointer is null or empty."));
        return;
    }

    FString Output = TEXT("Colors: ");
    for (int32 i = 0; i < Length; ++i)
    {
        Output += FString::Printf(TEXT("%d "), Colors[i]);
    }

    UE_LOG(LogTemp, Log, TEXT("%s"), *Output);
}

bool FRenderRequest::LoadColorsFromJsonToBuffer(const FString& FilePath, uint8_t*& OutColors, int32& OutColorCount)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath)) return false;

	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonString), Root) || !Root) return false;

	// Step 1: Find max ID
	uint8 MaxId = 0;
	Tags = Root->Values.Num();

	OutColorCount = Tags;
	OutColors = new uint8_t[OutColorCount * 3];
	FMemory::Memzero(OutColors, OutColorCount * 3);

	// Step 3: Populate Colors using id as index
	uint8 Id = 0;
	for (const auto& Pair : Root->Values)
	{
		TSharedPtr<FJsonObject> Entry = Pair.Value->AsObject();
		if (Entry.IsValid() && Entry->HasTypedField<EJson::Number>("id"))
		{
			// uint8 Id = static_cast<uint8>(Entry->GetIntegerField("id"));

			const TArray<TSharedPtr<FJsonValue>>* ColorArray;
			if (Entry->TryGetArrayField("color", ColorArray) && ColorArray->Num() == 3)
			{
				OutColors[Id * 3 + 0] = static_cast<uint8>((*ColorArray)[0]->AsNumber());
				OutColors[Id * 3 + 1] = static_cast<uint8>((*ColorArray)[1]->AsNumber());
				OutColors[Id * 3 + 2] = static_cast<uint8>((*ColorArray)[2]->AsNumber());

				Id ++;
			}
		}
	}

	return true;
}

uint8_t* FRenderRequest::GetPalette()
{

	FString FilePath = FPaths::ProjectDir() + TEXT("../../palette.json");
    FString JsonString;

	TSharedPtr<FJsonObject> JsonObject;

	uint8_t* Colors = nullptr;
	int32 ColorCount = 0;

	if (LoadColorsFromJsonToBuffer(FilePath, Colors, ColorCount))
	{
		// Colors[id * 3 + 0] = R, G, B for each id
	}
	
	return Colors;
}

void FRenderRequest::RetrievePixels(FColor* PixelBuffer, UTextureRenderTarget2D* Texture) {
	this->Buffer = PixelBuffer;
	this->TargetTexture = Texture;
	CheckNotBlockedOnRenderThread(); // not the issue

	// Queue up the task of rendering the scene in the render thread
	TGraphTask<FRenderRequest>::CreateTask().ConstructAndDispatchWhenReady(*this);
}

void FRenderRequest::RetrievePixels(FColor* PixelBuffer, UTextureRenderTarget2D* Texture, bool ShouldConvertToPalette) {
	this->Buffer = PixelBuffer;
	this->TargetTexture = Texture;
	this->ConvertToPalette = ShouldConvertToPalette;
	if(this->Palette == nullptr){
		this->Palette = GetPalette();
	}
	
	// this->Palette = HolooceanPalette();
	CheckNotBlockedOnRenderThread(); // not the issue

	// Queue up the task of rendering the scene in the render thread
	TGraphTask<FRenderRequest>::CreateTask().ConstructAndDispatchWhenReady(*this);
}



void FRenderRequest::ExecuteTask() const
{
	SCOPE_CYCLE_COUNTER(STAT_CameraExecuteTask);


	TArray<FColor> SurfaceData;
	FRHICommandListImmediate& RHICmdList = GetImmediateCommandList_ForRenderCommand();
	FTextureRenderTargetResource*  rt_resource = TargetTexture->GetRenderTargetResource();

	if (rt_resource != nullptr) {
		const FTexture2DRHIRef& rhi_texture = rt_resource->GetRenderTargetTexture();
		//FIntPoint size;
		FReadSurfaceDataFlags flags(RCM_UNorm, CubeFace_MAX);
		flags.SetLinearToGamma(false);

		{
			SCOPE_CYCLE_COUNTER(STAT_CameraExecuteTask_ReadSurfaceData);
			// This next call is slow! Significant impact on the frame time (~8ms)
			RHICmdList.ReadSurfaceData(
				rhi_texture,
				FIntRect(0, 0, TargetTexture->SizeX, TargetTexture->SizeY),
				SurfaceData,
				flags);
		}

		{
			SCOPE_CYCLE_COUNTER(STAT_CameraExecuteTask_Memcpy);

			FMemory::Memcpy(this->Buffer, &SurfaceData[0], SurfaceData.Num() * sizeof(FColor)); // this line isn't the problem
		}

	}
}

FRenderRequest::~FRenderRequest() {};