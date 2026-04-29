// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"

#include "ImagingSonar.h"

#include "ProfilingSonar.generated.h"

/**
 * @brief Profiling Sonar sensor simulation.
 * Inherits from the UImagingSonar class.
 * Utilizes the same complex acoustic raycasting and multipath engine as the Imaging Sonar, 
 * but restricts the acoustic beam to a very narrow elevation (1 degree) to generate 
 * high-resolution cross-sectional profiles of the underwater environment.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UProfilingSonar : public UImagingSonar
{
    GENERATED_BODY()
    
public:
    /**
     * @brief Default Constructor.
     */
    UProfilingSonar();

    /**
     * @brief Parses JSON parameters to set dynamic resolutions, enforcing profiling acoustic limits.
     * @param ParmsJson JSON string containing configuration.
     */
    virtual void ParseSensorParms(FString ParmsJson) override;

};