// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "BlueBoat.h"

/**
 * @brief Sets default values for the BlueBoat agent.
 * * Initializes the AI controller, physical properties (mass, volume, buoyancy center),
 * and shifts the 2 thruster locations based on the offset to the origin.
 */
ABlueBoat::ABlueBoat() {
    PrimaryActorTick.bCanEverTick = true;

    // Set the defualt controller
    AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.BlueBoatController"), NULL, LOAD_None, NULL);
    AutoPossessAI = EAutoPossessAI::PlacedInWorld;

    // This values are all pulled from the solidworks file
    this->CenterBuoyancy = FVector(0, 0, 10); 
    this->CenterMass = FVector(0, 0, 0);
    this->MassInKG = 200;
    this->OffsetToOrigin = FVector(0, 0, 20);
    this->Volume = 6 * MassInKG / WaterDensity; 
    
    this->BoundingBox = FBox(FVector(-250, -120, -25), FVector(250, 120, 25));

    // Shift thruster locations by offset to the origin
    for(int i=0;i<2;i++){
        thrusterLocations[i] += this->OffsetToOrigin;
    }
}

/**
 * @brief Initializes the agent and its physical components.
 * * Casts the RootComponent to a UStaticMeshComponent to allow physics manipulation
 * and calls the parent class initialization.
 */
void ABlueBoat::InitializeAgent() {
    RootMesh = Cast<UStaticMeshComponent>(RootComponent);

    Super::InitializeAgent();
}

/**
 * @brief Called every frame to apply physical forces to the boat.
 * * Converts linear and angular accelerations from the Python CommandArray into forces 
 * and torques, clamps them to maximum limits, and applies them to the RootMesh.
 * * @param DeltaSeconds The time elapsed since the last tick.
 */
void ABlueBoat::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    // Convert linear acceleration to force
    FVector linAccel = FVector(CommandArray[0], CommandArray[1], CommandArray[2]);
    linAccel = ClampVector(linAccel, -FVector(BB_MAX_LIN_ACCEL), FVector(BB_MAX_LIN_ACCEL));
    linAccel = ConvertLinearVector(linAccel, ClientToUE);

    // Convert angular acceleration to torque
    FVector angAccel = FVector(CommandArray[3], CommandArray[4], CommandArray[5]);
    angAccel = ClampVector(angAccel, -FVector(BB_MAX_ANG_ACCEL), FVector(BB_MAX_ANG_ACCEL));
    angAccel = ConvertAngularVector(angAccel, NoScale);


    RootMesh->GetBodyInstance()->AddForce(linAccel, true, true);
    RootMesh->GetBodyInstance()->AddTorqueInRadians(angAccel, true, true);
}

/**
 * @brief Enables linear and angular damping for the boat's physical mesh.
 * * For empty dynamics, damping is disabled. It should be enabled when using 
 * thrusters or a controller to simulate water resistance.
 */
void ABlueBoat::EnableDamping(){
    RootMesh->SetLinearDamping(3);
    RootMesh->SetAngularDamping(0.75);
}

/**
 * @brief Applies specific force values to the boat's thrusters.
 * * Iterates through the 2 thrusters, clamps the requested force to BB_MAX_THRUST,
 * converts the force vector to Unreal Engine's coordinate system, and applies it 
 * as a local force at the respective thruster locations.
 * * @param ThrusterArray Pointer to an array containing the requested force for each thruster.
 */
void ABlueBoat::ApplyThrusters(float* const ThrusterArray){
    // Iterate through 2 thrusters
    for(int i=0;i<2;i++){
        float force = FMath::Clamp(ThrusterArray[i], -BB_MAX_THRUST, BB_MAX_THRUST);

        FVector LocalForce = FVector(force, 0, 0);
        LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

        RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
    }
}