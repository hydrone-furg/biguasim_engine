// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "ImagingSonar.h"
#include <numeric> // Necessário para std::accumulate
// #pragma warning (disable : 4101)

/**
 * @brief Default Constructor. Initializes the sensor name.
 */
UImagingSonar::UImagingSonar() {
    SensorName = "ImagingSonar";
}

/**
 * @brief Cleans up dynamically allocated memory for the octree arrays.
 */
void UImagingSonar::BeginDestroy() {
    Super::BeginDestroy();

    delete[] count;
    delete[] hasPerfectNormal;
}

/**
 * @brief Parses the JSON configuration sent by the Python client to dynamically set sensor parameters.
 * Extracts noise settings (Sigma, Covariance), Multipath toggles, and resolution bins (Range, Azimuth, Elevation).
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UImagingSonar::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

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
        if (JsonParsed->HasTypedField<EJson::Boolean>("ScaleNoise")) {
            ScaleNoise = JsonParsed->GetBoolField("ScaleNoise");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AzimuthStreaks")) {
            AzimuthStreaks = JsonParsed->GetIntegerField("AzimuthStreaks");
        }

        // Multipath Settings
        if (JsonParsed->HasTypedField<EJson::Boolean>("MultiPath")) {
            MultiPath = JsonParsed->GetBoolField("MultiPath");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("ClusterSize")) {
            ClusterSize = JsonParsed->GetIntegerField("ClusterSize");
        }

        // Size of our binning
        if (JsonParsed->HasTypedField<EJson::Number>("RangeBins")) {
            RangeBins = JsonParsed->GetIntegerField("RangeBins");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("RangeRes")) {
            RangeRes = JsonParsed->GetNumberField("RangeRes")*100;
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AzimuthBins")) {
            AzimuthBins = JsonParsed->GetIntegerField("AzimuthBins");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("AzimuthRes")) {
            AzimuthRes = JsonParsed->GetNumberField("AzimuthRes");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("ElevationBins")) {
            ElevationBins = JsonParsed->GetIntegerField("ElevationBins");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("ElevationRes")) {
            ElevationRes = JsonParsed->GetNumberField("ElevationRes");
        }
    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("UImagingSonar::ParseSensorParms:: Unable to parse json."));
    }

    // Parse through the Range parameters given to us
    if(RangeBins != 0){
        RangeRes = (RangeMax - RangeMin) / RangeBins;
    } 
    else if(RangeRes != 0){
        RangeBins = (RangeMax - RangeMin) / RangeRes;
    }
    else{
        RangeBins = 512;
        RangeRes = (RangeMax - RangeMin) / RangeBins;
    }

    // Parse through the Azimuth parameters given to us
    if(AzimuthBins != 0){
        AzimuthRes = Azimuth / AzimuthBins;
    } 
    else if(AzimuthRes != 0){
        AzimuthBins = Azimuth / AzimuthRes;
    }
    else{
        AzimuthBins = 512;
        AzimuthRes = Azimuth / AzimuthBins;
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
        float dist = (RangeMax - RangeMin) / 8 + RangeMin;
        ElevationBins = (dist*Elevation*Pi/180) / Octree::OctreeMin;
        if(ElevationBins < 1) ElevationBins = 1;
        ElevationRes = Elevation / ElevationBins;
    }
}

/**
 * @brief Initializes the sensor matrices, calculates optimal azimuth bin scales for shadowing,
 * and sets up tracking for perfect reflections.
 */
void UImagingSonar::InitializeSensor() {
    Super::InitializeSensor();
    
    // Check if we should shadow with less Azimuth bins 
    float dist = (RangeMax - RangeMin) / 8 + RangeMin;
    AzimuthBinScale = 1;
    while(Octree::OctreeMin >= (dist*AzimuthRes*Pi/180)*AzimuthBinScale){
        AzimuthBinScale *= 2;
    }
    if(AzimuthBinScale > AzimuthBins) AzimuthBinScale = AzimuthBins;

    // setup count of each bin
    count = new int32[RangeBins*AzimuthBins]();
    hasPerfectNormal = new int32[AzimuthBins*RangeBins]();

    // Define a perfect reflection
    perfectCos = UKismetMathLibrary::DegCos(8);
    for(int i=0;i<ElevationBins*AzimuthBins/AzimuthBinScale;i++){
        sortedLeaves.Add(TArray<Octree*>());
        sortedLeaves[i].Reserve(10000);
    }

    mapLeaves.Reserve(100000);
}

/**
 * @brief Executed every tick to simulate acoustic wave propagation, generate noise, 
 * calculate multipath reflections, and output raw imagery and ground truth data.
 * @param DeltaTime Time since the last frame.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UImagingSonar::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);

    if(TickCounter == 0){
        int imageSize = RangeBins * AzimuthBins;
        // ---------------------------------------------------------
        // 1. MEMORY AND POINTER SETUP
        // ---------------------------------------------------------
        
        // Original pointer (with noise) [Channel 0]
        float* result = static_cast<float*>(Buffer);

        float* resultGT = nullptr;
        float* resultElev = nullptr;

        float* resultPX = nullptr; //channel of X
        float* resultPY = nullptr; //channel of Y
        float* resultPZ = nullptr; //channel of Z

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
            
            // Clear individually for safety
            std::fill(resultPX, resultPX + imageSize, 0.0f);
            std::fill(resultPY, resultPY + imageSize, 0.0f);
            std::fill(resultPZ, resultPZ + imageSize, 0.0f);
            currentChannel += 3;
        }

        // Clear main noisy buffer
        std::fill(result, result + imageSize, 0);

        // Temporary buffer for "Winner Takes All" elevation logic
        // Stores the maximum intensity found for each pixel
        float* maxIntensityBuffer = new float[imageSize](); 

        // Reset tracking variables
        std::fill(count, count+RangeBins*AzimuthBins, 0);
        std::fill(hasPerfectNormal, hasPerfectNormal+AzimuthBins*RangeBins, 0);
        
        for(auto& sl: sortedLeaves){
            sl.Reset();
        }
        mapLeaves.Reset();
        mapSearch.Reset();
        cluster.Reset();

        // ---------------------------------------------------------
        // 2. SEARCH AND SORT (OCTREE)
        // ---------------------------------------------------------

        // Finds leaves in range and puts them in foundLeaves
        findLeaves();       

        // SORT THEM INTO AZIMUTH/ELEVATION BINS
        int32 idx;
        for(TArray<Octree*>& bin : foundLeaves){
            for(Octree* l : bin){
                // Compute bins while we're parallelized
                l->idx.Y = (int32)((l->locSpherical.Y - minAzimuth)/ AzimuthRes);
                l->idx.Z = (int32)((l->locSpherical.Z - minElev)/ ElevationRes);
                // Sometimes we get float->int rounding errors
                if(l->idx.Y == AzimuthBins) --l->idx.Y;

                idx = l->idx.Z*AzimuthBins/AzimuthBinScale + l->idx.Y/AzimuthBinScale;
                sortedLeaves[idx].Emplace(l);
            }
        }

        // HANDLE SHADOWING
        shadowLeaves();

        // ---------------------------------------------------------
        // 3. MAIN PROCESSING LOOP
        // ---------------------------------------------------------
        float noise, pdf;
        int32 idxGT; 
        float valGT;

        for(TArray<Octree*>& bin : sortedLeaves){
            for(Octree* l : bin){
                
                // === A. GROUND TRUTH PROCESSING (NOISELESS) ===
                
                // Calculate clean X index (Pure geometric range)
                int32 idxX_GT = (int32)((l->locSpherical.X - RangeMin) / RangeRes);
                
                // Edge protection
                if (idxX_GT >= RangeBins) idxX_GT = RangeBins - 1;
                if (idxX_GT < 0) idxX_GT = 0;

                // Pure value from Octree (calculated in shadowLeaves)
                valGT = l->val; 
                // Linear index for GT
                idxGT = idxX_GT * AzimuthBins + l->idx.Y;
                
                // Elevation Logic: Pixel inherits angle of the strongest reflection (Winner Takes All)
                if (valGT > maxIntensityBuffer[idxGT]) {
                    maxIntensityBuffer[idxGT] = valGT;

                    // ONLY FILL ELEVATION IF FLAG IS ACTIVE
                    if (SendGTElevation && resultElev) {
                        resultElev[idxGT] = 90.0f - l->locSpherical.Z;
                    }

                    // ONLY FILL POINTCLOUD IF FLAG IS ACTIVE
                    if (SendGTPointCloud && resultPX) {
                        resultPX[idxGT] = l->loc.X / 100.0f;
                        resultPY[idxGT] = l->loc.Y / 100.0f;
                        resultPZ[idxGT] = l->loc.Z / 100.0f;
                    }
                }

                // ONLY ACCUMULATE INTENSITY IF FLAG IS ACTIVE
                if (SendGTIntensity && resultGT) {
                    resultGT[idxGT] += valGT;
                }

                // === B. NOISY PROCESSING (ORIGINAL) ===
                
                // Add noise to each of them
                noise = rNoise.sampleExponential();
                pdf = rNoise.exponentialScaledPDF(noise);
                
                // Modify leaf position based on noise
                l->idx.X = (int32)((l->locSpherical.X + noise - RangeMin) / RangeRes);
                
                // Modify leaf value (l->val is altered here, hence retrieving valGT earlier)
                l->val *= pdf; 

                // In case our noise has pushed us out of range
                if(l->idx.X >= RangeBins) l->idx.X = RangeBins-1;
                if(l->idx.X < 0) l->idx.X = 0;

                // Add to their appropriate bin
                idx = l->idx.X*AzimuthBins + l->idx.Y;
                if(l->cos > perfectCos) hasPerfectNormal[idx] += 1;

                result[idx] += l->val;
                ++count[idx];
            }
        }

        // ---------------------------------------------------------
        // 4. MULTIPATH (Applied only to Noisy image per GT definition)
        // ---------------------------------------------------------
        if(MultiPath){
            // PUT INTO MAP FOR CLUSTER
            for(TArray<Octree*>& binLeafs : sortedLeaves){
                if(binLeafs.Num() > 0){
                    // Get first element in this azimuth, elevation bin
                    Octree* jth = binLeafs.GetData()[0];
                    mapLeaves.Add(jth->idx, jth);
                    int idxR = jth->idx.X;
                    // Iterate through only taking ones with different range idx (idx.X)
                    // Note that the bin is sorted from shadowing above.
                    for(int i=1;i<binLeafs.Num();i++){
                        jth = binLeafs.GetData()[i];
                        if(jth->idx.X != idxR){
                            mapLeaves.Add(jth->idx, jth);
                            idxR = jth->idx.X;
                        }
                    }
                }
            }

            // PUT THEM INTO CLUSTERS
            mapSearch = TMap<FIntVector,Octree*>(mapLeaves);
            mapSearch.Compact();
            int i_start, j_start, k_start, i_end, j_end, k_end;
            Octree** close = nullptr;
            while(mapSearch.Num() > 0){
                // Get start of cluster
                Octree* l = mapSearch.begin()->Value;
                mapSearch.Remove(l->idx);
                cluster.Add({l});

                // Get anything that may be nearby
                i_start = FGenericPlatformMath::Max(0,l->idx.X-ClusterSize);
                j_start = FGenericPlatformMath::Max(0,l->idx.Y-ClusterSize);
                k_start = FGenericPlatformMath::Max(0,l->idx.Z-ClusterSize);
                i_end = FGenericPlatformMath::Min(RangeBins,l->idx.X+ClusterSize+1);
                j_end = FGenericPlatformMath::Min(AzimuthBins,l->idx.Y+ClusterSize+1);
                k_end = FGenericPlatformMath::Min(ElevationBins,l->idx.Z+ClusterSize+1);
                for(int i=i_start; i<i_end; i++){
                    for(int j=j_start; j<j_end; j++){
                        for(int k=k_start; k<k_end; k++){
                            close = mapSearch.Find(FIntVector(i,j,k));
                            if(close != nullptr && FVector::DotProduct(l->normal, (*close)->normal) > 0.965){
                                cluster.Top().Add(*close);
                                mapSearch.Remove((*close)->idx);
                            }
                        }
                    }
                }
            }


            // MULTIPATH CONTRIBUTIONS
            float step_size = Octree::OctreeMin;
            int iterations = RangeMax / Octree::OctreeMin;
            FTransform SensortoWorld = this->GetComponentTransform();
            std::function<FVector(FVector,FVector)> reflect;
            reflect = [](FVector normal, FVector impact){
                return -impact + 2*FVector::DotProduct(normal,impact)*normal;
            };
            ParallelFor(cluster.Num(), [&](int32 i){
                TArray<Octree*>& thisCluster = cluster.GetData()[i];
                Octree* l = thisCluster.GetData()[0];

                FVector reflection = reflect(l->normal, l->normalImpact);
                Octree stepper(l->loc, l->size);
                Octree** hit = nullptr; 
                FVector offset = reflection*step_size*30;

                // TODO: Replace this with real raytracing?
                for(int32 j=0;j<iterations;j++){
                    // step
                    offset += reflection*step_size;
                    stepper.loc = l->loc + offset;

                    // make sure it's still in range (& compute spherical coordinates)
                    if(!inRange(&stepper)){
                        thisCluster.Empty();
                        return;
                    }

                    // Set the index values
                    stepper.idx.X = (int32)((stepper.locSpherical.X - RangeMin) / RangeRes);
                    stepper.idx.Y = (int32)((stepper.locSpherical.Y - minAzimuth)/ AzimuthRes);
                    stepper.idx.Z = (int32)((stepper.locSpherical.Z - minElev)/ ElevationRes);

                    // If there's something in that bin
                    hit = mapLeaves.Find(stepper.idx);
                    if(hit != nullptr){
                        FVector returnImpact = reflect((*hit)->normal, -reflection);
                        // make sure it's in the right direction
                        if(FVector::DotProduct(returnImpact, (*hit)->normalImpact) > 0) break;
                        else {
                            thisCluster.Empty();
                            return;
                        }
                    }
                } 

                if(hit == nullptr){
                    thisCluster.Empty();
                    return;
                }

                // If we did hit something, ray trace the rest of everything in the cluster
                float t, noise, pdf, R1, R2;
                FVector locBounce, returnRay;
                for(Octree* m : thisCluster){
                    // find 2nd impact location
                    reflection = reflect(m->normal, m->normalImpact);
                    t = FVector::DotProduct((*hit)->loc - m->loc, (*hit)->normal) / (FVector::DotProduct(reflection, (*hit)->normal));
                    locBounce = m->loc + reflection*t;

                    // find return vector
                    // TODO: See if any change in accuracy in just using the hit version, should be pretty close angles

                    // find ray return
                    returnRay = reflect((*hit)->normal, -reflection);

                    // find spherical location
                    Octree bounce(locBounce, m->size);
                    inRange(&bounce);
                    bounce.locSpherical.X += m->locSpherical.X + FVector::Dist(bounce.loc, m->loc);
                    bounce.locSpherical.X /= 2;

                    // Convert to contribution index
                    noise = rNoise.sampleExponential();
                    pdf = rNoise.exponentialScaledPDF(noise);
                    m->idx.X = (int32)((bounce.locSpherical.X + noise - RangeMin) / RangeRes);
                    m->idx.Y = (int32)((bounce.locSpherical.Y - minAzimuth)/ AzimuthRes);
                    m->cos = FVector::DotProduct(returnRay, (*hit)->normalImpact);
                    R1 = (m->z - WaterImpedance) / (m->z + WaterImpedance);
                    R2 = ((*hit)->z - WaterImpedance) / ((*hit)->z + WaterImpedance);
                    m->val = R1*R1*R2*R2*m->cos*pdf;

                    // TODO: There's a bug this is working around, find it and fix it
                    if(m->idx.X < 0) m->idx.X = 0;
                    if(m->idx.X > RangeBins) m->idx.X = RangeBins-1;
                    if(m->idx.Y < 0) m->idx.Y = 0;
                    if(m->idx.Y > AzimuthBins) m->idx.Y = AzimuthBins-1;

                    // APPLY ONLY TO NOISY SIGNAL
                    idx = m->idx.X*AzimuthBins + m->idx.Y;
                    result[idx] += m->val;
                    ++count[idx];
                }
            }, false);

            // Add direct cluster contributions to the noisy result
            for(TArray<Octree*>& bin : cluster){
                for(Octree* l : bin){
                    idx = l->idx.X*AzimuthBins + l->idx.Y;

                    result[idx] += l->val;
                    ++count[idx];
                }
            }
        }


        // ---------------------------------------------------------
        // 5. NORMALIZATION AND FINAL PERTURBATION
        // ---------------------------------------------------------
        float scale_range, scale_total, azimuth;
        float std = Azimuth/64;
        int idxGT_Norm;

        for (int i=0; i<RangeBins; i++) {
            // Scale along range to recreate intensity dropoff
            scale_range = i*RangeRes/RangeMax;
            scale_range = scale_range*scale_range;
            for(int j=0; j<AzimuthBins; j++){
                // Scale along azimuth to recreat lobe shape
                azimuth = j*AzimuthRes - Azimuth/2;
                scale_total = scale_range*(1 + UKismetMathLibrary::Exp(-azimuth*azimuth/std)*0.5);

                if(!ScaleNoise) scale_total = 1;

                idx = i*AzimuthBins + j;
                idxGT_Norm = idx;

                // --- GT PROCESSING (Channel 1) ---
                // Applies beam pattern and distance attenuation, but NO stochastic noise
                if (resultGT) {
                    resultGT[idx] *= scale_total;
                    
                    // If the signal is too weak, invalidate auxiliary data
                    if(resultGT[idx] <= 0.00001f) {
                        if (resultElev) resultElev[idx] = -999.0f; 
                        if (resultPX) {
                            resultPX[idx] = 0.0f;
                            resultPY[idx] = 0.0f;
                            resultPZ[idx] = 0.0f;
                        }
                    }
                }
                // If there was a signal, resultElev already contains the angle of the strongest object

                // --- NOISY PROCESSING (Channel 0) ---
                // Normalize & perturb
                if(count[idx] != 0){
                    result[idx] *= (0.5 + multNoise.sampleFloat())/count[idx];
                    result[idx] += addNoise.sampleRayleigh()*scale_total;
                }
                else{
                    result[idx] = addNoise.sampleRayleigh()*scale_total;
                }
            }
        }

        // CHECK IF ROWS HAVE STREAKING ISSUES
        // (Applies only to noisy channel to simulate real sensor defects)
        if(AzimuthStreaks == -1 || AzimuthStreaks == 1){
            float percToBand = 0.08;
            float avgPerfect;
            int numPerfect, numTotal;
            for(int i=0; i<RangeBins; i++){
                // Count how many in that row have dead on normals
                numPerfect = std::accumulate(hasPerfectNormal+i*AzimuthBins, hasPerfectNormal+(i+1)*AzimuthBins, 0);
                numTotal = std::accumulate(count+i*AzimuthBins, count+(i+1)*AzimuthBins, 0);
                avgPerfect = numTotal == 0 ? 0 : (float)numPerfect / (float)numTotal;  

                // If there's enough, shallow out those bounds
                if(avgPerfect >= percToBand){
                    for(int j=0; j<AzimuthBins; j++){
                        idx = i*AzimuthBins + j;

                        // Attempts to remove streak
                        if(AzimuthStreaks == -1){
                            result[idx] = result[idx]*result[idx];
                        }
                        // Adding in streak
                        else if(AzimuthStreaks == 1){
                            result[idx] = 1 - (1- result[idx])*(1- result[idx]);
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------
        // 6. MEMORY CLEANUP
        // ---------------------------------------------------------
        delete[] maxIntensityBuffer;
    }
}