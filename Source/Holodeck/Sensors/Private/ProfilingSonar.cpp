// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "ProfilingSonar.h"

/**
 * @brief Default Constructor.
 * Sets the sensor name to ProfilingSonar.
 */
UProfilingSonar::UProfilingSonar() {
    SensorName = "ProfilingSonar";
}

/**
 * @brief Parses the JSON configuration sent by the Python client.
 * Intercepts the configuration to enforce the physical characteristics of a Profiling Sonar 
 * (extremely narrow 1-degree elevation beam, and specific min/max ranges) before passing 
 * the payload to the parent ImagingSonar class. Also sets high-resolution default bins 
 * if the user doesn't provide them.
 * @param ParmsJson The JSON string containing the sensor configuration.
 */
void UProfilingSonar::ParseSensorParms(FString ParmsJson) {
    // Force acoustic profile for a Profiling Sonar
    Elevation = 1;
    RangeMin = 0.5 * 100; // 0.5 meters to cm
    RangeMax = 75 * 100;  // 75 meters to cm

    TSharedPtr<FJsonObject> JsonParsed;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
        // If they haven't set RangeRes or RangeBins, input our default RangeBins
        if (!JsonParsed->HasTypedField<EJson::Number>("RangeRes") && !JsonParsed->HasTypedField<EJson::Number>("RangeBins")) {
            RangeBins = 750;
        }

        // If they haven't set AzimuthRes or AzimuthBins, input our default AzimuthBins
        if (!JsonParsed->HasTypedField<EJson::Number>("AzimuthRes") && !JsonParsed->HasTypedField<EJson::Number>("AzimuthBins")) {
            AzimuthBins = 480;
        }
    }

    // Pass the parsed JSON to the complex ImagingSonar backend
    Super::ParseSensorParms(ParmsJson);
}