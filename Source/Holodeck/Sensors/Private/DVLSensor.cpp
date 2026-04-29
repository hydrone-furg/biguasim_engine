// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "DVLSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name and enables ticking every frame.
 */
UDVLSensor::UDVLSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    SensorName = "DVLSensor";
}

/**
 * @brief Parses the JSON configuration sent by the Python client to dynamically set sensor parameters.
 * Extracts the beam elevation angle, max range, debug flags, and multivariate noise parameters (Sigma and Covariance) 
 * for both velocity and range measurements.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UDVLSensor::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

        if (JsonParsed->HasTypedField<EJson::Number>("Elevation")) {
            elevation = JsonParsed->GetNumberField("Elevation");
        }
        if (JsonParsed->HasTypedField<EJson::Boolean>("ReturnRange")) {
            ReturnRange = JsonParsed->GetBoolField("ReturnRange");
        }
        if (JsonParsed->HasTypedField<EJson::Number>("MaxRange")) {
            MaxRange = JsonParsed->GetNumberField("MaxRange")*100; // Convert to cm
        }
        if (JsonParsed->HasTypedField<EJson::Boolean>("DebugLines")) {
            DebugLines = JsonParsed->GetBoolField("DebugLines");
        }

        // For handling noise
        if (JsonParsed->HasTypedField<EJson::Number>("VelSigma")) {
            mvnVel.initSigma(JsonParsed->GetNumberField("VelSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("VelSigma")) {
            mvnVel.initSigma(JsonParsed->GetArrayField("VelSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("VelCov")) {
            mvnVel.initCov(JsonParsed->GetNumberField("VelCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("VelCov")) {
            mvnVel.initCov(JsonParsed->GetArrayField("VelCov"));
        }

        if (JsonParsed->HasTypedField<EJson::Number>("RangeSigma")) {
            mvnRange.initSigma(JsonParsed->GetNumberField("RangeSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("RangeSigma")) {
            mvnRange.initSigma(JsonParsed->GetArrayField("RangeSigma"));
        }
        if (JsonParsed->HasTypedField<EJson::Number>("RangeCov")) {
            mvnRange.initCov(JsonParsed->GetNumberField("RangeCov"));
        }
        if (JsonParsed->HasTypedField<EJson::Array>("RangeCov")) {
            mvnRange.initCov(JsonParsed->GetArrayField("RangeCov"));
        }

    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("UDVLSensor::ParseSensorParms:: Unable to parse json."));
    }
}

/**
 * @brief Initializes the sensor matrices and computes the Janus configuration transformation.
 * Uses the elevation angle to build the mathematical matrix that maps the 4 acoustic beams 
 * to the vehicle's local 3D velocity vectors.
 */
void UDVLSensor::InitializeSensor() {
    Super::InitializeSensor();

    // You need to get the pointer to the object the sensor is attached to. 
    Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());

    sinElev = UKismetMathLibrary::DegSin(elevation);
    cosElev = UKismetMathLibrary::DegCos(elevation);

    // Make transformation matrix based on Janus configuration
    // Reference: https://etda.libraries.psu.edu/files/final_submissions/17327
    transform = {   {1/(2*sinElev),             0, -1/(2*sinElev),             0},
                    {            0, 1/(2*sinElev),              0, -1/(2*sinElev)},
                    {1/(4*cosElev), 1/(4*cosElev),  1/(4*cosElev),  1/(4*cosElev)} };

    // Make direction lines for the 4 beams
    directions = {FVector(sinElev, 0, -cosElev),
                FVector(0, -sinElev, -cosElev),
                FVector(-sinElev, 0, -cosElev),
                FVector(0, sinElev, -cosElev)};
                
    // Scale up to point MaxRange distance
    for(FVector& d : directions) d *= MaxRange;
}

/**
 * @brief Main execution loop that calculates the true relative velocity and beam ranges.
 * Queries the physics engine for linear velocity, transforms it to the sensor's local frame,
 * adds simulated acoustic noise, and optionally performs 4 line traces to simulate acoustic 
 * pings bouncing off the sea floor.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UDVLSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    
    // Check if your parent pointer is valid, and if the sensor is on. 
    // Then get the velocity and buffer, and send the data to it. 
    if (Parent != nullptr && bOn) {
        float* FloatBuffer = static_cast<float*>(Buffer);
        FTransform SensortoWorld = this->GetComponentTransform();
        FVector Location = this->GetComponentLocation();

        // UE4 gives us world velocity, rotate it to get local velocity
        FVector Velocity = Parent->GetPhysicsLinearVelocityAtPoint(this->GetComponentLocation());
        Velocity = SensortoWorld.GetRotation().UnrotateVector(Velocity);
        Velocity = ConvertLinearVector(Velocity, UEToClient);

        // Add noise if it's been enabled
        if(mvnVel.isUncertain()){
            TArray<float> sample = mvnVel.sampleTArray();
            for(int i=0;i<4;i++){
                Velocity.X += transform[0][i]*sample[i];
                Velocity.Y += transform[1][i]*sample[i];
                Velocity.Z += transform[2][i]*sample[i];
            }
        }

        // Send X, Y, Z velocity to buffer
        FloatBuffer[0] = Velocity.X;
        FloatBuffer[1] = Velocity.Y;
        FloatBuffer[2] = Velocity.Z;

        // Get beam ranges if requested
        if(ReturnRange){
            // Get parameters we'll need
            FCollisionQueryParams QueryParams = FCollisionQueryParams();
            QueryParams.AddIgnoredActor(this->GetAttachmentRootActor());

            // Iterate through and do all raytracing for the 4 beams
            for(int i=0;i<4;i++){
                FVector end = SensortoWorld.TransformPositionNoScale(directions[i]);
                FHitResult Hit = FHitResult();
                bool TraceResult = GetWorld()->LineTraceSingleByChannel(Hit, Location, end, ECollisionChannel::ECC_Visibility, QueryParams);
                FloatBuffer[i+3] = (TraceResult ? Hit.Distance : MaxRange) / 100.0f;  // Convert centimeter to meters
            }

            // Add range noise if it's been enabled
            if(mvnRange.isUncertain()){
                TArray<float> sample = mvnRange.sampleTArray();
                for(int i=0;i<4;i++){
                    FloatBuffer[i+3] += sample[i];
                }
            } 
        }

        // Display debug lines in the editor if requested
        if(DebugLines){
            for(int i=0;i<4;i++){
                FVector end = SensortoWorld.TransformPositionNoScale(directions[i]);
                DrawDebugLine(GetWorld(), Location, end, FColor::Green, false, .01, ECC_WorldStatic, 1.f);
            }
        }
    }
}