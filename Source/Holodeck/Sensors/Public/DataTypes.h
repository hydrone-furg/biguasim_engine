#include "Holodeck.h"
#include "HolodeckCamera.h"
#include "DetectionComponent.h"

#include <cstdint>
#include <vector>


class FVisualData {
public:
    FColor* Pixel;
    TArray<FDetectionInfo>* Detections;

    FVisualData(FColor* pixels, TArray<FDetectionInfo>* detections)
    {
        Pixel = pixels;
        Detections = detections;
    };

};