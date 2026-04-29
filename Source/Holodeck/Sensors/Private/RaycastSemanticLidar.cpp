#include "RaycastSemanticLidar.h"
#include <cmath>

/**
 * @brief Default Constructor.
 * Sets the sensor name.
 */
URaycastSemanticLidar::URaycastSemanticLidar()
{
    SensorName = "RaycastSemanticLidar";
    Parent = nullptr;
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Extracts physical Lidar properties such as channels, field of view, rotation frequency,
 * noise models, and drop-off rates.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void URaycastSemanticLidar::ParseSensorParms(FString ParmsJson)
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
 * @brief Initializes the sensor and sets up the semantic data buffer.
 */
void URaycastSemanticLidar::InitializeSensor()
{
    Super::InitializeSensor();

    if (ShouldInitialize)
    {
        ShouldInitialize = false;
        this->SemanticLidarBuffer = static_cast<float*>(Buffer);
        this->SemanticLidarData = FSemanticLidarData(this->SemanticLidarBuffer, Description.Channels);
    }
    
    Parent = TUniquePtr<AActor>(this->GetAttachmentRootActor());
}

/**
 * @brief Main execution loop that fires the Lidar simulation.
 */
void URaycastSemanticLidar::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    SimulateLidar(DeltaTime);
}

/**
 * @brief Applies the parsed description parameters to the sensor's physical model.
 * Resizes the internal point channels and generates the laser angles.
 * @param LidarDescription The struct containing all parsed Lidar specifications.
 */
void URaycastSemanticLidar::Set(const FLidarDescription& LidarDescription)
{
    Description = LidarDescription;
    UE_LOG(LogHolodeck, Error, TEXT("Range: %f"), Description.Range)

    CreateLasers();
    PointsPerChannel.resize(Description.Channels);
}

/**
 * @brief Computes the vertical angles for each laser channel based on the FOV limits.
 */
void URaycastSemanticLidar::CreateLasers()
{
    const auto NumberOfLasers = Description.Channels;
    check(NumberOfLasers > 0u);

    const float DeltaAngle = NumberOfLasers == 1u ? 0.f :
        (Description.UpperFovLimit - Description.LowerFovLimit) /
        static_cast<float>(NumberOfLasers - 1);

    LaserAngles.Empty(NumberOfLasers);

    for (auto i = 0u; i < NumberOfLasers; ++i)
    {
        const float VerticalAngle = Description.UpperFovLimit - static_cast<float>(i) * DeltaAngle;
        LaserAngles.Emplace(VerticalAngle);
    }
}

/**
 * @brief Executes the parallelized raycasting logic to simulate the rotating Lidar head.
 * @param DeltaTime The time elapsed since the last tick.
 */
void URaycastSemanticLidar::SimulateLidar(const float DeltaTime)
{
    const uint32 ChannelCount = Description.Channels;
    const uint32 PointsToScanWithOneLaser = FMath::RoundHalfFromZero(
            Description.PointsPerSecond * DeltaTime / static_cast<float>(ChannelCount)
        );

    if (PointsToScanWithOneLaser <= 0)
    {
        UE_LOG(LogHolodeck, Warning, TEXT("%s: no points request this frame.. Try increasing the number of points per second."), *GetName());
        return;
    }
    
    check(ChannelCount == LaserAngles.Num());

    const float CurrentHorizontalAngle = FMath::RadiansToDegrees(SemanticLidarData.GetHorizontalAngle());
    const float AngleDistanceOfTick = Description.RotationFrequency * Description.HorizontalFov * DeltaTime;
    const float AngleDistanceOfLaserMeasure = AngleDistanceOfTick / PointsToScanWithOneLaser;

    ResetRecordedHits(ChannelCount, PointsToScanWithOneLaser);
    PreprocessRays(ChannelCount, PointsToScanWithOneLaser);

    {
        SCOPE_CYCLE_COUNTER(STAT_ParallelForHoloocean)
        // Highly optimized parallel loop to shoot thousands of rays simultaneously
        ParallelFor(ChannelCount, [&](int32 idxChannel) {
          TRACE_CPUPROFILER_EVENT_SCOPE(STAT_ParallelFor);
            
          FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("Laser_Trace")), true, Parent.Get());
          TraceParams.bTraceComplex = true;
          TraceParams.bReturnPhysicalMaterial = false;

          for (auto idxPtsOneLaser = 0u; idxPtsOneLaser < PointsToScanWithOneLaser; idxPtsOneLaser++) {
            FHitResult HitResult;
            const float VertAngle = LaserAngles[idxChannel];
            const float HorizAngle = std::fmod(CurrentHorizontalAngle + AngleDistanceOfLaserMeasure
                * idxPtsOneLaser, Description.HorizontalFov) - Description.HorizontalFov / 2;
            const bool PreprocessResult = RayPreprocessCondition[idxChannel][idxPtsOneLaser];

            if (PreprocessResult && ShootLaser(VertAngle, HorizAngle, HitResult, TraceParams)) {
              WritePointAsync(idxChannel, HitResult);
            }
          };
        });
    }

    FTransform ActorTransf = Parent->GetTransform();
    ComputeAndSaveDetections(ActorTransf);

    const float HorizontalAngle = FMath::RadiansToDegrees(std::fmod(CurrentHorizontalAngle + AngleDistanceOfTick, Description.HorizontalFov));
    SemanticLidarData.SetHorizontalAngle(HorizontalAngle);
}

/**
 * @brief Clears the hit recording structures for a new frame.
 */
void URaycastSemanticLidar::ResetRecordedHits(uint32_t Channels, uint32_t MaxPointsPerChannel)
{
    RecordedHits.resize(Channels);

    for (auto& hits : RecordedHits)
    {
        hits.clear();
        hits.reserve(MaxPointsPerChannel);
    }
}

/**
 * @brief Resets the boolean conditions for whether a ray should be fired.
 */
void URaycastSemanticLidar::PreprocessRays(uint32_t Channels, uint32_t MaxPointsPerChannel)
{
    RayPreprocessCondition.resize(Channels);

    for (auto& conds : RayPreprocessCondition)
    {
        conds.clear();
        conds.resize(MaxPointsPerChannel);
        std::fill(conds.begin(), conds.end(), true);
    }
}

/**
 * @brief Asynchronously writes a successful hit to the temporary recording buffer.
 */
void URaycastSemanticLidar::WritePointAsync(uint32_t Channel, FHitResult& Detection)
{
    RecordedHits[Channel].emplace_back(Detection);
}

/**
 * @brief Compiles the raw hits into the final memory buffer.
 * Processes each hit to extract semantic information and writes to the shared memory array.
 * @param SensorTransform The global transform of the Lidar sensor.
 */
void URaycastSemanticLidar::ComputeAndSaveDetections(const FTransform& SensorTransform)
{
    for (auto idxChannel = 0u; idxChannel < Description.Channels; ++idxChannel)
    {
        PointsPerChannel[idxChannel] = RecordedHits[idxChannel].size();
    }
    SemanticLidarData.ResetMemory(PointsPerChannel);
    
    int PointCount = 0;
    for (auto idxChannel = 0u; idxChannel < Description.Channels; ++idxChannel)
    {
        for (auto& hit : RecordedHits[idxChannel])
        {
            FSemanticDetection detection;
            ComputeRawDetection(hit, SensorTransform, detection);
            SemanticLidarData.WritePointSync(detection);
            PointCount++;
        }
    }
    // Store the total number of valid points at the start of the buffer
    SemanticLidarBuffer[0] = PointCount;
}

/**
 * @brief Extracts geometry and semantic data from a physical raycast hit.
 * Retrieves the point location, the cosine of the incidence angle, and extracts the 
 * CustomDepthStencilValue (set by the AnnotationComponent) to serve as the Semantic Tag.
 * @param HitInfo The physics hit result from Unreal Engine.
 * @param SensorTransf The global transform of the Lidar sensor.
 * @param Detection The semantic detection object to be populated.
 */
void URaycastSemanticLidar::ComputeRawDetection(const FHitResult& HitInfo, const FTransform& SensorTransf, FSemanticDetection& Detection) const
{
    const FVector HitPoint = HitInfo.ImpactPoint;
    Detection.point = static_cast<FVector3f>(SensorTransf.Inverse().TransformPosition(HitPoint));
    
    const FVector VecInc = - (HitPoint - SensorTransf.GetLocation()).GetSafeNormal();
    Detection.cos_inc_angle = FVector::DotProduct(VecInc, HitInfo.ImpactNormal);
    
    const AActor* actor = HitInfo.GetActor();
    
    Detection.object_idx = 2;
    // EXTRACTS SEMANTIC TAG: Relies on AnnotationComponent having tagged the world
    Detection.object_tag = static_cast<uint32_t>(HitInfo.Component->CustomDepthStencilValue);
}

/**
 * @brief Shoots a single laser ray into the world physics engine.
 * @param VerticalAngle The pitch angle of the laser.
 * @param HorizontalAngle The yaw angle of the laser.
 * @param HitResult The struct to hold the physics impact data if a hit occurs.
 * @param TraceParams Configuration for the raycast (e.g., ignoring self).
 * @return True if the laser hit a blocking object, False otherwise.
 */
bool URaycastSemanticLidar::ShootLaser(const float VerticalAngle, float HorizontalAngle, FHitResult& HitResult, FCollisionQueryParams& TraceParams) const
{
    FHitResult HitInfo(ForceInit);
    FTransform ActorTransf = this->GetComponentTransform();
    FVector LidarBodyLoc = ActorTransf.GetLocation();

    FRotator LidarBodyRot = ActorTransf.Rotator();

    FRotator LaserRot(VerticalAngle, HorizontalAngle, 0); // Pitch, Yaw, Roll
    FRotator ResultRot = UKismetMathLibrary::ComposeRotators(LaserRot, LidarBodyRot);

    const auto Range = Description.Range;
    FVector EndTrace = Range * UKismetMathLibrary::GetForwardVector(ResultRot) + LidarBodyLoc;
    
    GetWorld()->LineTraceSingleByChannel(
        HitInfo,
        LidarBodyLoc,
        EndTrace,
        ECC_Visibility,
        TraceParams,
        FCollisionResponseParams::DefaultResponseParam
    );    
    
    if (HitInfo.bBlockingHit)
    {
        HitResult = HitInfo;
        return true;
    }

    return false;
}