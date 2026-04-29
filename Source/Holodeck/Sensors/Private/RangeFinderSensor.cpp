// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "RangeFinderSensor.h"
#include "Json.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name.
 */
URangeFinderSensor::URangeFinderSensor() {
    SensorName = "RangeFinderSensor";
}

/**
 * @brief Parses the JSON configuration sent by the Python client to dynamically set sensor parameters.
 * Extracts the number of lasers, pitch angle, max range, and debug visibility.
 * Converts the angle to match the client's expectation (positive is up) and distance from meters to cm.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void URangeFinderSensor::ParseSensorParms(FString ParmsJson) {
    Super::ParseSensorParms(ParmsJson);

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

        if (JsonParsed->HasTypedField<EJson::Number>("LaserCount")) {
            LaserCount = JsonParsed->GetIntegerField("LaserCount");
        }

        if (JsonParsed->HasTypedField<EJson::Number>("LaserAngle")) {
            LaserAngle = -JsonParsed->GetNumberField("LaserAngle");  // in client positive angles point up
        }

        if (JsonParsed->HasTypedField<EJson::Number>("LaserMaxDistance")) {
            LaserMaxDistance = JsonParsed->GetNumberField("LaserMaxDistance") * 100.0f;  // meters to centimeters
        }

        if (JsonParsed->HasTypedField<EJson::Boolean>("LaserDebug")) {
            LaserDebug = JsonParsed->GetBoolField("LaserDebug");
        }
    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("URangeFinderSensor::ParseSensorParms:: Unable to parse json."));
    }
}

/**
 * @brief Initializes the sensor and fetches the root actor to ignore self-collisions.
 */
void URangeFinderSensor::InitializeSensor() {
    Super::InitializeSensor();
    // You need to get the pointer to the object you are attached to. 
    Parent = this->GetAttachmentRootActor();
}

/**
 * @brief Main execution loop that calculates raycasts and distances.
 * Iterates through the configured number of lasers, calculating the rotation 
 * for a 360-degree sweep (or a cone if LaserAngle is set), casts the rays, 
 * and stores the collision distances in meters into the shared memory buffer.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void URangeFinderSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

    float* FloatBuffer = static_cast<float*>(Buffer);

    for (int i = 0; i < LaserCount; i++) {

        FVector start = GetComponentLocation();

        FVector end = GetForwardVector();
        FVector right = GetRightVector();
        
        // Distribute lasers evenly in a 360 degree circle
        end = end.RotateAngleAxis(-360.0f * i / LaserCount, GetUpVector());
        right = right.RotateAngleAxis(-360.0f * i / LaserCount, GetUpVector());
        
        // Apply pitch offset to turn the plane into a cone
        end = end.RotateAngleAxis(LaserAngle, right);
        
        // Scale to max distance and translate to world space
        end = end * LaserMaxDistance;
        end = start + end;

        FCollisionQueryParams QueryParams = FCollisionQueryParams();
        QueryParams.AddIgnoredActor(Parent);

        FHitResult Hit = FHitResult();

        // Perform the raycast
        bool TraceResult = GetWorld()->LineTraceSingleByChannel(Hit, start, end, ECollisionChannel::ECC_Visibility, QueryParams);
        
        // Write the result in meters (or max distance if no hit)
        FloatBuffer[i] = (TraceResult ? Hit.Distance : LaserMaxDistance) / 100.0f;

        if (LaserDebug) {
            DrawDebugLine(GetWorld(), start, end, FColor::Green, false, .01, ECC_WorldStatic, 1.f);
        }
    }
}