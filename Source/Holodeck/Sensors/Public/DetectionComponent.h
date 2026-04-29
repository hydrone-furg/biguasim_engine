// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Holodeck.h"
#include "HolodeckCamera.h"
#include "ObjectFilter.h"

#include "DetectionComponent.generated.h"

USTRUCT()
struct FDetectionInfo
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* Actor;

    UPROPERTY()
    FBox2D Box2D;

    UPROPERTY()
    FBox Box3D;

    UPROPERTY()
    FTransform RelativeTransform;

};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UDetectionComponent : public UHolodeckCamera 
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UDetectionComponent();

    virtual void InitializeSensor() override;

    // Called every frame
    // Called every frame
    virtual void ParseSensorParms(FString ParmsJson) override;

    int GetNumItems() override { return gb_detections; };
	int GetItemSize() override { return sizeof(std::string); };

protected:
    virtual void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    

    // const TArray<FDetectionInfo>& getDetections(std::string* buffer);
    void getDetections(std::string* buffer);

    void addMeshName(const std::string& mesh_name);
    void setFilterRadius(const float radius_cm);
    void clearMeshNames();
    
private:

    bool calcBoundingFromViewInfo(AActor* actor, FBox2D& box_out);

    FVector getRelativeLocation(FVector in_location);

    FRotator getRelativeRotation(FVector in_location, FRotator in_rotation);

    std::string utf8_encode(const std::wstring &wstr);

    std::wstring string_to_wstring(const std::string &str);

    char* Buffer;   


    AActor* Parent;
    
    UPROPERTY()
    FObjectFilter object_filter_;

    UPROPERTY(EditAnywhere, Category = "Tracked Actors")
    float max_distance_to_camera_;

    UPROPERTY()
    UTextureRenderTarget2D* texture_target_;

    UPROPERTY()
    USceneCaptureComponent2D* scene_capture_component_2D_;

    UPROPERTY(EditAnywhere)
	FString target_detection_;

    UPROPERTY(EditAnywhere)
	bool ReturnRange = false;

    UPROPERTY(EditAnywhere)
	int gb_detections = 10000000;

    UPROPERTY()
    TArray<FDetectionInfo> cached_detections_;

    
    
};
