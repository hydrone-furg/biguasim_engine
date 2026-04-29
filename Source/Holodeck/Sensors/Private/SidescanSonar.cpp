// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "SidescanSonar.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name.
 */
USidescanSonar::USidescanSonar() {
    SensorName = "SidescanSonar";
}

/**
 * @brief Cleans up dynamically allocated memory for the bin counting array.
 */
void USidescanSonar::BeginDestroy() {
    Super::BeginDestroy();

    delete[] count;
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Overrides the base UHolodeckSonar default parameters to fit a standard Sidescan profile 
 * (170-degree azimuth fan, extremely narrow 0.25-degree elevation). Dynamically calculates 
 * the necessary bin resolutions if not explicitly provided.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void USidescanSonar::ParseSensorParms(FString ParmsJson) {

    // Override the Parent Class defaults for some key parameters
    Azimuth = 170;          // degrees
    Elevation = 0.25;       // degrees
    RangeMin = 0.5 * 100;   // 0.5 m (in cm)
    RangeMax = 35 * 100;    // 35 m (in cm)

    // Parse any parent class parameters that were provided in the configuration
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

        // For handling noise
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

        if (JsonParsed->HasTypedField<EJson::Number>("RangeRes")) {
            RangeRes = JsonParsed->GetNumberField("RangeRes") * 100; // m to cm
        }
        if (JsonParsed->HasTypedField<EJson::Number>("ElevationRes")) {
            ElevationRes = JsonParsed->GetNumberField("ElevationRes"); // degrees
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AzimuthRes")) {
            AzimuthRes = JsonParsed->GetNumberField("AzimuthRes"); // degrees
        }
        if (JsonParsed->HasTypedField<EJson::Number>("RangeBins")) {
            RangeBins = JsonParsed->GetIntegerField("RangeBins");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AzimuthBins")) {
            AzimuthBins = JsonParsed->GetIntegerField("AzimuthBins");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("ElevationBins")) {
            ElevationBins = JsonParsed->GetIntegerField("ElevationBins");
        }
    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("USidescanSonar::ParseSensorParms:: Unable to parse json."));
    }

    // Parse through the Range parameters given to us
    if(RangeBins != 0){
        RangeRes = (RangeMax - RangeMin) / RangeBins;
    } 
    else if(RangeRes != 0){
        RangeBins = (RangeMax - RangeMin) / RangeRes;
    }
    else{
        RangeRes = 5; // cm
        RangeBins = (RangeMax - RangeMin) / RangeRes;
    }

    // Parse through the Azimuth parameters given to us
    if(AzimuthBins != 0){
        AzimuthRes = Azimuth / AzimuthBins;
    } 
    else if(AzimuthRes != 0){
        AzimuthBins = Azimuth / AzimuthRes;
    }
    else{
        // Calculate how large the azimuth bins should be
        AzimuthRes = (180 * Octree::OctreeMin) / (Pi * (RangeMin + 0.1 * (RangeMax - RangeMin)));
        AzimuthBins = Azimuth / AzimuthRes;
    }

    // Parse through the Elevation parameters given to us
    if(ElevationBins != 0){
        ElevationRes = Elevation / ElevationBins;
    } 
    else if(ElevationRes != 0){
        ElevationBins = Elevation / ElevationRes;
    }
    else{
        // Calculate how large our shadowing bins should be
        ElevationBins = (RangeMin*Elevation*Pi/180) / Octree::OctreeMin;
        if(ElevationBins < 1) ElevationBins = 1;
        ElevationRes = Elevation / ElevationBins;
    }
}

/**
 * @brief Initializes the sensor matrices and octree tracking arrays.
 * Sets up a 1D counting array for the Sidescan output format.
 */
void USidescanSonar::InitializeSensor() {
    Super::InitializeSensor();
    
    // Setup count of each bin
    count = new int32[RangeBins](); // Sidescan Sonar (1D array)

    for(int i=0;i<AzimuthBins*ElevationBins;i++){
        sortedLeaves.Add(TArray<Octree*>());
        sortedLeaves[i].Reserve(10000);
    }
}

/**
 * @brief Helper function to convert Spherical coordinates back to Euclidean world space.
 */
FVector spherToEucSS(float r, float theta, float phi, FTransform SensortoWorld){
    float x = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegCos(theta);
    float y = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegSin(theta);
    float z = r*UKismetMathLibrary::DegCos(phi);
    return UKismetMathLibrary::TransformLocation(SensortoWorld, FVector(x, y, z));
}

/**
 * @brief Main execution loop to simulate acoustic wave propagation for a Sidescan profile.
 * Extracts a 1D cross-sectional array representing the port and starboard acoustic returns.
 * Also handles Ground Truth generation (PointCloud and Elevation mapping).
 */
void USidescanSonar::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);

    if(TickCounter == 0){
        // Sidescan "image" size is strictly 1D (RangeBins)
        int imageSize = RangeBins;
        float* result = static_cast<float*>(Buffer);
        
        // Reset things and get ready
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

        // Clear noisy buffer (Channel 0)
        std::fill(result, result+RangeBins, 0);
        std::fill(count, count+RangeBins, 0);
        
        // Auxiliary buffer for "Winner Takes All" (Elev and PC)
        float* maxIntensityBuffer = new float[imageSize]();
        
        for(auto& sl: sortedLeaves){
            sl.Reset();
        }

        // Finds leaves in range and puts them in foundLeaves
        findLeaves();       

        // Sort them into Azimuth/Elevation bins
        int32 idx;
        for(TArray<Octree*>& bin : foundLeaves){
            for(Octree* l : bin){
                l->idx.Y = (int32)((l->locSpherical.Y - minAzimuth)/ AzimuthRes);
                l->idx.Z = (int32)((l->locSpherical.Z - minElev)/ ElevationRes);
                if(l->idx.Y == AzimuthBins) --l->idx.Y;

                idx = l->idx.Z*AzimuthBins + l->idx.Y;
                sortedLeaves[idx].Emplace(l);
            }
        }

        // Handle Shadowing (blocking sound propagation behind objects)
        shadowLeaves();

        // Add in all contributions, separating port and starboard into a 1D array
        for(TArray<Octree*>& bin : sortedLeaves){
            for(Octree* l : bin){
                // Calculate range bin
                l->idx.X = (int32)((l->locSpherical.X - RangeMin) / RangeRes);

                // Split into Port/Starboard array logic
                if (l->idx.Y > (AzimuthBins / 2)){
                    idx = RangeBins / 2 - l->idx.X / 2 - 1;
                }
                else{
                    idx = RangeBins / 2 + l->idx.X / 2;
                }

                // Index bounds protection
                if(idx < 0) idx = 0;
                if(idx >= RangeBins) idx = RangeBins - 1;

                float valGT = l->val;

                // Ground Truth Filling
                if (SendGTIntensity && resultGT) {
                    resultGT[idx] += valGT; // Accumulate like real sonar
                }

                // Winner Takes All for Elevation and PointCloud
                if (valGT > maxIntensityBuffer[idx]) {
                    maxIntensityBuffer[idx] = valGT;

                    if (SendGTElevation && resultElev) {
                        resultElev[idx] = 90.0f - l->locSpherical.Z;
                    }

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

        // Normalize the noisy buffer
        for (int i = 0; i < RangeBins; i++) {
            if(count[i] != 0){
                result[i] *= (1 + multNoise.sampleFloat()) / count[i];
                result[i] += addNoise.sampleRayleigh();
            }
            else{
                result[i] = addNoise.sampleRayleigh();
            }
        }
        
        delete[] maxIntensityBuffer;
    }

    // Diagnostics warning for extremely thin elevation slices 
    if (runtickCounter == 20 && (RangeMin*Elevation*Pi/180) / Octree::OctreeMin < 1)
    {
        float recommendedElevation = Octree::OctreeMin * 180 / (RangeMin * Pi);
        float recommendedOctreeMin = RangeMin * Elevation * Pi / 180 / 100;
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("WARNING: Elevation angle potentially too small with current OctreeMin configuration\n Recommended changes (pick one):\n Elevation = %f\n OctreeMin = %f\n"), recommendedElevation, recommendedOctreeMin));
    }

    runtickCounter++;
}