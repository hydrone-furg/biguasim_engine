// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "SinglebeamSonar.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name.
 */
USinglebeamSonar::USinglebeamSonar() {
    SensorName = "SinglebeamSonar";
} 

/**
 * @brief Cleans up dynamically allocated memory for the bin counting array.
 */
void USinglebeamSonar::BeginDestroy() {
    Super::BeginDestroy();

    delete[] count;
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Configures the conical geometry of the beam (OpeningAngle), sets the resolution bins, 
 * and initializes the mathematical noise distributions.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void USinglebeamSonar::ParseSensorParms(FString ParmsJson) {

    // Default range in cm (0.5m to 10m)
    RangeMin = 0.5 * 100;
    RangeMax = 10 * 100;

    Super::ParseSensorParms(ParmsJson);

    // User can override default values
    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
        // Geometry Parameters
        if (JsonParsed->HasTypedField<EJson::Number>("OpeningAngle")) {
            OpeningAngle = JsonParsed->GetNumberField("OpeningAngle");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("RangeBins")) {
            RangeBins = JsonParsed->GetIntegerField("RangeBins");
        }   
        if (JsonParsed->HasTypedField<EJson::Number>("RangeRes")) {
            RangeRes = JsonParsed->GetNumberField("RangeRes")*100;
        }

        // Advanced Params (Binning for the conical geometry)
        if (JsonParsed->HasTypedField<EJson::Number>("OpeningAngleBins")) {
            OpeningAngleBins = JsonParsed->GetIntegerField("OpeningAngleBins");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("OpeningAngleRes")) {
            OpeningAngleRes = JsonParsed->GetNumberField("OpeningAngleRes");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("CentralAngleBins")) {
            CentralAngleBins = JsonParsed->GetIntegerField("CentralAngleBins");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("CentralAngleRes")) {
            CentralAngleRes = JsonParsed->GetNumberField("CentralAngleRes");
        }

        if (!JsonParsed->HasTypedField<EJson::Number>("ShadowEpsilon")) {
            ShadowEpsilon = Octree::OctreeMin;
        }

        // Noise Parameters
        if (JsonParsed->HasTypedField<EJson::Number>("AddSigma")) {
            addNoise.initSigma(JsonParsed->GetNumberField("AddSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AddCov")) {
            addNoise.initCov(JsonParsed->GetNumberField("AddCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("MultSigma")) {
            multNoise.initSigma(JsonParsed->GetNumberField("MultSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("MultCov")) {
            multNoise.initCov(JsonParsed->GetNumberField("MultCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("RangeSigma")) {
            rNoise.initBounds(JsonParsed->GetNumberField("RangeSigma")*100);
        }
    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("USinglebeamSonar::ParseSensorParms:: Unable to parse json."));
    }

    // Dynamic resolution calculations...
    if(RangeBins != 0) RangeRes = (RangeMax - RangeMin) / RangeBins;
    else if(RangeRes != 0) RangeBins = (RangeMax - RangeMin) / RangeRes;
    else { RangeBins = 200; RangeRes = (RangeMax - RangeMin) / RangeBins; }

    if(OpeningAngleBins != 0) OpeningAngleRes = OpeningAngle / OpeningAngleBins;
    else if(OpeningAngleRes != 0) OpeningAngleBins = OpeningAngle / OpeningAngleRes;
    else {
        float dist = RangeMin;
        OpeningAngleBins = (dist*OpeningAngle*Pi/180) / Octree::OctreeMin;
        if(OpeningAngleBins < 1) OpeningAngleBins = 1;
        OpeningAngleRes = OpeningAngle / OpeningAngleBins;
    }

    if(CentralAngleBins != 0) CentralAngleRes = CentralAngle / CentralAngleBins;
    else if(CentralAngleRes != 0) CentralAngleBins = CentralAngle / CentralAngleRes;
    else {
        float dist = RangeMin;
        CentralAngleBins = (dist*CentralAngle*Pi/180) / Octree::OctreeMin;
        if(CentralAngleBins < 6) CentralAngleBins = 6;
        CentralAngleRes = CentralAngle / CentralAngleBins;
    }
}

/**
 * @brief Initializes the sensor matrices and mathematical caches for the conical spread.
 */
void USinglebeamSonar::InitializeSensor() {
    Super::InitializeSensor();
    
    CentralAngle = 360; // Full 360 degree rotation around the forward axis
    minCentralAngle = -180;
    maxCentralAngle = 180;
    minOpeningAngle = 0;
    maxOpeningAngle = OpeningAngle / 2; // Spread outwards from the center axis
    
    count = new int32[RangeBins]();

    for(int i=0;i<CentralAngleBins*OpeningAngleBins;i++){
        sortedLeaves.Add(TArray<Octree*>());
        sortedLeaves[i].Reserve(10000);
    }

    sqrt3_2 = UKismetMathLibrary::Sqrt(3)/2;
    sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(CentralAngle, OpeningAngle)/2);
}

/**
 * @brief Determines if an Octree leaf falls inside the 3D conical acoustic beam.
 * Translates global coordinates to the sensor's local frame and evaluates against the 
 * Opening Angle (cone spread) and Central Angle (rotation).
 * @param tree The Octree node to evaluate.
 * @return True if the leaf is inside the acoustic cone, False otherwise.
 */
bool USinglebeamSonar::inRange(Octree* tree){
    FTransform SensortoWorld = this->GetComponentTransform();
    float offset = 0;
    float radius = 0;

    if(tree->size != Octree::OctreeMin){
        radius = tree->size*sqrt3_2;
        offset = radius/sinOffset;
        SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
    }
    
    // Transform location to sensor frame
    FVector locLocal = SensortoWorld.GetRotation().UnrotateVector(tree->loc-SensortoWorld.GetTranslation());

    // Check Range
    tree->locSpherical.X = locLocal.Size();
    if(RangeMin+offset-radius >= tree->locSpherical.X || tree->locSpherical.X >= RangeMax+offset+radius) return false; 

    // Check OpeningAngle (Angle off of X-axis)
    if(UseApprox){
        tree->locSpherical.Z = ATan2Approx(UKismetMathLibrary::Sqrt(UKismetMathLibrary::Square(locLocal.Y)+UKismetMathLibrary::Square(locLocal.Z)), locLocal.X); 
    } else {
        tree->locSpherical.Z = UKismetMathLibrary::DegAtan2(UKismetMathLibrary::Sqrt(UKismetMathLibrary::Square(locLocal.Y)+UKismetMathLibrary::Square(locLocal.Z)), locLocal.X);
    }
    if(minOpeningAngle >= tree->locSpherical.Z || tree->locSpherical.Z >= maxOpeningAngle) return false;

    // Save CentralAngle (Angle around the X-axis) for shadowing later
    if(UseApprox){
        tree->locSpherical.Y = ATan2Approx(locLocal.Z, locLocal.Y);
    } else {
        tree->locSpherical.Y = UKismetMathLibrary::DegAtan2(locLocal.Z, locLocal.Y);
    }
    
    return true;
}   

/**
 * @brief Draws a debug cone in the Unreal Editor representing the physical acoustic beam.
 */
void USinglebeamSonar::showRegion(float DeltaTime){
    if(ViewRegion){
        float debugThickness = 3.0f;
        float DebugNumSides = 6; 
        float length = (RangeMax - RangeMin); 
        DrawDebugCone(GetWorld(), GetComponentLocation(), GetForwardVector(), length, (OpeningAngle/2)*Pi/180, (OpeningAngle/2)*Pi/180, DebugNumSides, FColor::Green, false, .00, ECC_WorldStatic, debugThickness);
    }       
}

/**
 * @brief Main execution loop to simulate acoustic wave propagation for a Singlebeam profile.
 * Aggregates all acoustic returns within the conical beam into a single 1D array representing 
 * the intensity profile over distance.
 */
void USinglebeamSonar::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);

    if(TickCounter == 0){
        int imageSize = RangeBins;
        float* result = static_cast<float*>(Buffer);

        float* resultGT = nullptr;
        float* resultElev = nullptr;
        float* resultPX = nullptr; float* resultPY = nullptr; float* resultPZ = nullptr;

        int currentChannel = 1;

        if (SendGTIntensity) {
            resultGT = result + (currentChannel * imageSize);
            std::fill(resultGT, resultGT + imageSize, 0.0f);
            currentChannel++;
        }

        if (SendGTElevation) {
            resultElev = result + (currentChannel * imageSize);
            std::fill(resultElev, resultElev + imageSize, -999.0f);
            currentChannel++;
        }

        if (SendGTPointCloud) {
            resultPX = result + (currentChannel * imageSize);
            resultPY = result + ((currentChannel + 1) * imageSize);
            resultPZ = result + ((currentChannel + 2) * imageSize);
            
            std::fill(resultPX, resultPX + imageSize, 0.0f);
            std::fill(resultPY, resultPY + imageSize, 0.0f);
            std::fill(resultPZ, resultPZ + imageSize, 0.0f);
            currentChannel += 3;
        }

        std::fill(result, result+RangeBins, 0);
        std::fill(count, count+RangeBins, 0);
        
        float* maxIntensityBuffer = new float[imageSize]();

        for(auto& sl: sortedLeaves) sl.Reset();

        findLeaves();

        // Sort into CentralAngle/OpeningAngle Bins
        int32 idx;
        for(TArray<Octree*>& bin : foundLeaves){
            for(Octree* l : bin){
                l->idx.Y = (int32)((l->locSpherical.Y - minCentralAngle)/ CentralAngleRes);
                l->idx.Z = (int32)((l->locSpherical.Z - minOpeningAngle)/ OpeningAngleRes);
                if(l->idx.Y == CentralAngleBins) --l->idx.Y;

                idx = l->idx.Z*CentralAngleBins + l->idx.Y;
                sortedLeaves[idx].Emplace(l);
            }
        }

        shadowLeaves();

        // Add all contributions into the 1D Range array
        float range_noise;
        for(TArray<Octree*>& bin : sortedLeaves){
            for(Octree* l : bin){
                range_noise = rNoise.sampleExponential();
                l->idx.X = (int32)((l->locSpherical.X - RangeMin + range_noise) / RangeRes); 

                if(l->idx.X < 0) l->idx.X = 0;
                if(l->idx.X >= RangeBins) l->idx.X = RangeBins-1;

                idx = l->idx.X;
                float valGT = l->val;

                if (SendGTIntensity && resultGT) resultGT[idx] += valGT;

                if (valGT > maxIntensityBuffer[idx]) {
                    maxIntensityBuffer[idx] = valGT;
                    if (SendGTElevation && resultElev) resultElev[idx] = 0; // Elevation is meaningless inside a narrow cone
                    
                    if (SendGTPointCloud && resultPX) {
                        resultPX[idx] = l->loc.X / 100.0f;
                        resultPY[idx] = l->loc.Y / 100.0f;
                        resultPZ[idx] = l->loc.Z / 100.0f;
                    }
                }

                result[idx] += l->val;
                ++count[idx];
            }
        }
        
        // Normalize and apply noise
        for (int i = 0; i < RangeBins; i++) {
            if(count[i] != 0){
                result[i] *= (1 + multNoise.sampleFloat())/count[i];
                result[i] += addNoise.sampleRayleigh();
            }
            else{
                result[i] = addNoise.sampleRayleigh();
            }
        }   
        
        delete[] maxIntensityBuffer;
    }

    TickCounter++;
    if(TickCounter % TicksPerCapture == 0 && octree != nullptr && toMake.Num() == 0){
        TickCounter = 0;
    }
}