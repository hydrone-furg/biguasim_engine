// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "BlueROV2Heavy.h"

/**
 * @brief Sets default values for the BlueROV2 Heavy agent.
 * * Initializes the AI controller, physical properties (mass, volume, buoyancy center),
 * and the physical offset to align the Unreal Engine origin with the model's center.
 */
ABlueROV2Heavy::ABlueROV2Heavy() {
    PrimaryActorTick.bCanEverTick = true;
    // Set the defualt controller
    AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.BlueROV2Controller"), NULL, LOAD_None, NULL);
    AutoPossessAI = EAutoPossessAI::PlacedInWorld;

    // This values are all pulled from the solidworks file
    this->Volume = .03554577;   
    this->CenterBuoyancy = FVector(-5.96, 0.29, -1.85); 
    this->CenterMass = FVector(-5.9, 0.46, -2.82);
    this->MassInKG = 31.02;
    this->OffsetToOrigin = FVector(-0.7, -2, 32);
}

/**
 * @brief Initializes the agent and its physical components.
 * * Casts the RootComponent to a UStaticMeshComponent to allow physics manipulation.
 * If the 'Perfect' flag is true, recalculates optimal center of mass and buoyancy.
 * Also applies the OffsetToOrigin to all 8 thruster locations.
 */
void ABlueROV2Heavy::InitializeAgent() {
    RootMesh = Cast<UStaticMeshComponent>(RootComponent);

    if(Perfect){
        this->CenterMass = (thrusterLocations[0] + thrusterLocations[2]) / 2;
        this->CenterMass.Z = thrusterLocations[7].Z;
        
        this->CenterBuoyancy = CenterMass;
        this->CenterBuoyancy.Z += 5;

        this->Volume = MassInKG / WaterDensity;
    }

    // Apply OffsetToOrigin to all of our custom position vectors
    for(int i=0;i<8;i++){
        thrusterLocations[i] += OffsetToOrigin;
    }

    Super::InitializeAgent();
}

/**
 * @brief Called every frame to apply physical forces to the Heavy ROV.
 * * Converts linear and angular accelerations from the Python CommandArray into forces 
 * and torques, clamps them to maximum limits, and applies them to the RootMesh.
 * @param DeltaSeconds The time elapsed since the last tick.
 */
void ABlueROV2Heavy::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    // Convert linear acceleration to force
    FVector linAccel = FVector(CommandArray[0], CommandArray[1], CommandArray[2]);
    linAccel = ClampVector(linAccel, -FVector(BR_HEAVY_MAX_LIN_ACCEL), FVector(BR_HEAVY_MAX_LIN_ACCEL));
    linAccel = ConvertLinearVector(linAccel, ClientToUE);

    // Convert angular acceleration to torque
    FVector angAccel = FVector(CommandArray[3], CommandArray[4], CommandArray[5]);
    angAccel = ClampVector(angAccel, -FVector(BR_HEAVY_MAX_ANG_ACCEL), FVector(BR_HEAVY_MAX_ANG_ACCEL));
    angAccel = ConvertAngularVector(angAccel, NoScale);

    RootMesh->GetBodyInstance()->AddForce(linAccel, true, true);
    RootMesh->GetBodyInstance()->AddTorqueInRadians(angAccel, true, true);
}

/**
 * @brief Enables linear and angular damping for the Heavy ROV's physical mesh.
 */
void ABlueROV2Heavy::EnableDamping(){
    RootMesh->SetLinearDamping(1.0);
    RootMesh->SetAngularDamping(0.75);
}

/**
 * @brief Applies specific force values to the 8 thrusters of the BlueROV2 Heavy.
 * * Iterates through the 4 vertical thrusters (indices 0-3) applying vertical forces, 
 * and the 4 angled/vectored thrusters (indices 4-7) applying split X/Y forces.
 * @param ThrusterArray Pointer to an array containing the requested force for each thruster.
 */
void ABlueROV2Heavy::ApplyThrusters(float* const ThrusterArray){
    // Iterate through vertical thrusters
    for(int i=0;i<4;i++){
        float force = FMath::Clamp(ThrusterArray[i], -BR_HEAVY_MAX_THRUST, BR_HEAVY_MAX_THRUST);
        FVector LocalForce = FVector(0, 0, force);
        LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

        RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
    }

    // Iterate through angled thrusters
    for(int i=4;i<8;i++){
        float force = FMath::Clamp(ThrusterArray[i], -BR_HEAVY_MAX_THRUST, BR_HEAVY_MAX_THRUST);
        // 4 + 6 have negative y
        FVector LocalForce = FVector(0, 0, 0);
        if(i % 2 == 0)  
            LocalForce = FVector(force/UKismetMathLibrary::Sqrt(2), force/UKismetMathLibrary::Sqrt(2), 0);
        else    
            LocalForce = FVector(force/UKismetMathLibrary::Sqrt(2), -force/UKismetMathLibrary::Sqrt(2), 0);
        LocalForce = ConvertLinearVector(LocalForce, ClientToUE);
        RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
    }

}