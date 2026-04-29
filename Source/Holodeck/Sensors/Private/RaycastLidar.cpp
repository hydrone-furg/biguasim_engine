// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "RaycastLidar.h"
#include <cmath>
#include <Engine/CollisionProfile.h>
#include "RandomEngine.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and initializes the random engine used for noise and drop-off simulation.
 */
URaycastLidar::URaycastLidar()
{
    SensorName = "RaycastLidar";
    Parent = nullptr;
    DropOffAlpha = 0.0f;
    DropOffBeta = 0.0f;
    DropOffGenActive = false;
    
    RandomEngine = CreateDefaultSubobject<URandomEngine>(TEXT("RandomEngine"));
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Extracts physical Lidar properties such as channels, field of view, rotation frequency,
 * atmospheric attenuation, noise models, and intensity drop-off rates.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void URaycastLidar::ParseSensorParms(FString ParmsJson)
{
    Description = FLidarDescription();

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
    {
        JsonParsed->TryGetNumberField("Channels", Description.Channels);
        JsonParsed->TryGetNumberField("Range", Description.Range);
        JsonParsed->TryGetNumberField("PointsPerSecond", Description.PointsPerSecond);
        JsonParsed->TryGetNumberField("RotationFrequency", Description.RotationFrequency);
        JsonParsed->TryGetNumberField("UpperFovLimit", Description.UpperFovLimit);
        JsonParsed->TryGetNumberField("LowerFovLimit", Description.LowerFovLimit);
        JsonParsed->TryGetNumberField("HorizontalFov", Description.HorizontalFov);
        JsonParsed->TryGetNumberField("AtmospAttenRate", Description.AtmospAttenRate);
        JsonParsed->TryGetNumberField("RandomSeed", Description.RandomSeed);
        JsonParsed->TryGetNumberField("DropOffGenRate", Description.DropOffGenRate);
        JsonParsed->TryGetNumberField("DropOffIntensityLimit", Description.DropOffIntensityLimit);
        JsonParsed->TryGetNumberField("DropOffAtZeroIntensity", Description.DropOffAtZeroIntensity);
        JsonParsed->TryGetBoolField("ShowDebugPoints", Description.ShowDebugPoints);
        JsonParsed->TryGetNumberField("NoiseStdDev", Description.NoiseStdDev);
        Description.Range *= 100; // Convert meters to centimeters (Unreal Engine scale)
    }
    Set(Description);
}

/**
 * @brief Initializes the sensor and sets up the data buffer.
 * Bypasses the base class initialization (`Super::ShouldInitialize = false`) to handle 
 * its own custom raycast structures.
 */
void URaycastLidar::InitializeSensor()
{
    Super::ShouldInitialize = false;

    Super::InitializeSensor();

    Parent = TUniquePtr<AActor>(this->GetAttachmentRootActor());
    
    this->Buffer = static_cast<float*>(Super::Buffer);
    this->Data = FLidarData(this->Buffer);
}

/**
 * @brief Main execution loop that fires the Lidar simulation.
 */
void URaycastLidar::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    SimulateLidar(DeltaTime);
}

/**
 * @brief Applies the parsed description parameters to the sensor's physical model.
 * Resizes the internal point channels and computes the mathematical alpha/beta slopes 
 * for the intensity drop-off model.
 * @param LidarDescription The struct containing all parsed Lidar specifications.
 */
void URaycastLidar::Set(const FLidarDescription& LidarDescription)
{
    Description = LidarDescription;
    UE_LOG(LogHolodeck, Error, TEXT("Range: %f"), Description.Range)

    CreateLasers();
    PointsPerChannel.resize(Description.Channels);

    // Compute drop off model parameters
    DropOffBeta = 1.0f - Description.DropOffAtZeroIntensity;
    DropOffAlpha = Description.DropOffAtZeroIntensity / Description.DropOffIntensityLimit;
    DropOffGenActive = Description.DropOffGenRate > std::numeric_limits<float>::epsilon();
}

/**
 * @brief Computes the received intensity of a raw semantic detection point.
 * Simulates atmospheric attenuation over distance.
 * @param RawDetection The un-processed semantic hit.
 * @return The calculated return intensity.
 */
float URaycastLidar::ComputeIntensity(const FSemanticDetection& RawDetection) const
{
    const FVector3f HitPoint = RawDetection.point;
    const float Distance = HitPoint.Length();
    UE_LOG(LogHolodeck, Error, TEXT("Compute Intensity Distance: %f"), Distance)
    
    const float AttenAtm = Description.AtmospAttenRate;
    const float AbsAtm = exp(-AttenAtm * Distance);

    const float IntRec = AbsAtm;

    return IntRec;
}

/**
 * @brief Computes a valid Lidar detection from a physical raycast hit.
 * Transforms the hit point into local sensor space and calculates atmospheric intensity.
 * @param HitInfo The physics hit result from Unreal Engine.
 * @param SensorTransf The global transform of the Lidar sensor.
 * @return A structured Lidar detection containing the point and its intensity.
 */
URaycastLidar::FDetection URaycastLidar::ComputeDetection(const FHitResult& HitInfo, const FTransform& SensorTransf) const
{
    FDetection Detection;
    const FVector HitPoint = HitInfo.ImpactPoint;
    Detection.point = SensorTransf.Inverse().TransformPosition(HitPoint);

    const float Distance = Detection.point.Length();
    const float AttenAtm = Description.AtmospAttenRate;
    const float AbsAtm = exp(-AttenAtm * Distance);

    const float IntRec = AbsAtm;

    Detection.intensity = IntRec;

    return Detection;
}

/**
 * @brief Pre-processes the rays before firing to apply general drop-off probabilities.
 * @param Channels Number of vertical laser channels.
 * @param MaxPointsPerChannel The maximum number of points a single channel can fire per frame.
 */
void URaycastLidar::PreprocessRays(uint32_t Channels, uint32_t MaxPointsPerChannel)
{
    Super::PreprocessRays(Channels, MaxPointsPerChannel);

    for (auto ch = 0u; ch < Channels; ch++)
    {
        for (auto p = 0u; p < MaxPointsPerChannel; p++)
        {
            RayPreprocessCondition[ch][p] = !(DropOffGenActive && RandomEngine->GetUniformFloat() < Description.DropOffGenRate);
        }
    }
}

/**
 * @brief Post-processes a successful detection to apply physical noise and intensity drop-off.
 * Introduces Gaussian noise based on distance and evaluates if the point should be discarded 
 * because its intensity is too low to be read by the sensor.
 * @param Detection The detection point to process (modified in place).
 * @return True if the point survives the drop-off filter and should be kept; False if it is lost.
 */
bool URaycastLidar::PostprocessDetection(FDetection& Detection) const
{
    if (Description.NoiseStdDev > std::numeric_limits<float>::epsilon())
    {
        const auto ForwardVector = Detection.point.GetSafeNormal();
        UE_LOG(LogHolodeck, Error, TEXT("%f, %f, %f"), ForwardVector.X, ForwardVector.Y, ForwardVector.Z);
        const auto Noise = ForwardVector * RandomEngine->GetNormalDistribution(0.0f, Description.NoiseStdDev);
        Detection.point += Noise;
    }

    const float Intensity = Detection.intensity;
    if (Intensity > Description.DropOffIntensityLimit)
    {
        return true;
    }

    return RandomEngine->GetUniformFloat() < DropOffAlpha * Intensity + DropOffBeta;
}

/**
 * @brief Compiles the raw hits into the final memory buffer.
 * Iterates through all recorded hits, applies post-processing (noise/drop-off), 
 * and writes the surviving points to the shared memory array. The very first 
 * float in the buffer stores the total count of valid points.
 * @param SensorTransform The global transform of the Lidar sensor.
 */
void URaycastLidar::ComputeAndSaveDetections(const FTransform& SensorTransform)
{
    for (auto idxChannel = 0u; idxChannel < Description.Channels; ++idxChannel)
    {
        PointsPerChannel[idxChannel] = RecordedHits[idxChannel].size();
    }
    
    Data.ResetMemory(PointsPerChannel);
    int PointCount = 0;
    
    for (auto idxChannel = 0u; idxChannel < Description.Channels; ++idxChannel)
    {
        for (auto& hit : RecordedHits[idxChannel])
        {
            FDetection Detection = ComputeDetection(hit, SensorTransform);
            if (PostprocessDetection(Detection))
            {
                Data.WritePointSync(Detection);
                PointCount++;
            }
            else
            {
                PointsPerChannel[idxChannel]--;
            }
        }
    }
    // Store the total number of valid points at the start of the buffer
    Buffer[0] = PointCount;
}