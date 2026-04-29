// MIT License (c) 2019 BYU PCCL see LICENSE file
#include "Holodeck.h"
#include "DjiMatrice.h"
#include <cmath>

/**
 * @brief Sets default values for the DJI Matrice agent.
 * Initializes the AI controller, ticking, and collision settings.
 */
ADjiMatrice::ADjiMatrice() {
    // Set this pawn to call Tick() every frame.
    PrimaryActorTick.bCanEverTick = true;

    // Set the defualt controller
    AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.DjiMatriceController"), NULL, LOAD_None, NULL);
    AutoPossessAI = EAutoPossessAI::PlacedInWorld;
    
    SetActorEnableCollision(true);
    bForcesInMain = true;
}

/**
 * @brief Initializes the UAV physical properties.
 * Calculates gravity, mass, center of mass, and the lever arm length for torque calculations.
 */
void ADjiMatrice::InitializeAgent() {
    Super::InitializeAgent();
    RootMesh = Cast<UStaticMeshComponent>(RootComponent);
    Gravity = GWorld->GetGravityZ() / 100.0f;
    MassInKG = 15.0f;
    
    CenterMass = (thrusterLocations[0] + thrusterLocations[2]) / 2.0f;
    CenterMass.Z = thrusterLocations[0].Z;

    ArmLength = sqrt(pow(thrusterLocations[0][0], 2) + pow(thrusterLocations[0][1], 2));
}

/**
 * @brief Called every frame to apply linear and angular acceleration.
 * @param DeltaTime The time elapsed since the last tick.
 */
void ADjiMatrice::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    // Convert linear acceleration to force
    FVector linAccel = FVector(CommandArray[0], CommandArray[1], CommandArray[2]);
    linAccel = ConvertLinearVector(linAccel, ClientToUE);

    // Convert angular acceleration to torque
    FVector angAccel = FVector(CommandArray[3], CommandArray[4], CommandArray[5]);
    angAccel = ConvertAngularVector(angAccel, NoScale);

    RootMesh->GetBodyInstance()->AddForce(linAccel, true, true);
    RootMesh->GetBodyInstance()->AddTorqueInRadians(angAccel, true, true);
}

/**
 * @brief Toggles whether aerodynamic forces are calculated in the main loop.
 */
void ADjiMatrice::ForcesInMainController(bool SetIt){
    bForcesInMain = SetIt;
}

/**
 * @brief Applies drag and gravity to the UAV based on current velocity.
 */
void ADjiMatrice::ApplyFlightForce(){
    FVector Velocity = RootMesh->GetPhysicsLinearVelocity();
    float Speed = Velocity.Size();  
    FVector DragDirection = Velocity.GetSafeNormal();

    FVector DragForce = -0.5f * M_AIR_DENSITY * M_DRAG_COEFF * 0.4f * Speed * DragDirection;
    RootMesh->AddForce(DragForce);

    FVector GravityVector = ConvertLinearVector(FVector(0, 0, Gravity*MassInKG), ClientToUE);
    RootMesh->AddForceAtLocation(GravityVector, RootMesh->GetCenterOfMass());
}

/**
 * @brief Applies thrust and calculates torque for the 4 drone rotors.
 * Uses a thrust curve equation to map percentage to force, then calculates 
 * the resulting roll, pitch, and yaw torques based on the arm length.
 * @param ThrusterArray Pointer to an array of requested percentages (0-100) for each rotor.
 */
void ADjiMatrice::ApplyThrusters(float* const ThrusterArray){
    float pct0 = FMath::Clamp(ThrusterArray[0], 0.0f, 100.0f);
    float pct1 = FMath::Clamp(ThrusterArray[1], 0.0f, 100.0f);
    float pct2 = FMath::Clamp(ThrusterArray[2], 0.0f, 100.0f);
    float pct3 = FMath::Clamp(ThrusterArray[3], 0.0f, 100.0f);

    float t0 = (pct0 >= 30) ? GetThrust(pct0) : 0;
    float t1 = (pct1 >= 30) ? GetThrust(pct1) : 0;
    float t2 = (pct2 >= 30) ? GetThrust(pct2) : 0;
    float t3 = (pct3 >= 30) ? GetThrust(pct3) : 0;

    float f1 = ArmLength * (t1 + t3);
    float f2 = ArmLength * (t0 + t2);
    float f3 = ArmLength * (t0 + t1);
    float f4 = ArmLength * (t2 + t3);

    float t_phi = f1 - f2;
    float t_theta = f3 - f4;
    float t_psi = t0 + t3 - t1 - t2;
    float thrust = t0 + t1 + t2 + t3;

    FVector LocalThrust = FVector(0, 0, thrust);
    LocalThrust = ConvertLinearVector(LocalThrust, ClientToUE);
    FVector LocalTorque = FVector(t_phi, t_theta, t_psi);
    LocalTorque = ConvertLinearVector(LocalTorque, ClientToUE);

    // Apply torques and forces in global coordinates
    RootMesh->AddTorqueInRadians(GetActorRotation().RotateVector(LocalTorque));
    RootMesh->AddForce(GetActorRotation().RotateVector(LocalThrust));
}