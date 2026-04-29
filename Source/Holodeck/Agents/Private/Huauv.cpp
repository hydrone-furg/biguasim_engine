// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Huauv.h"

// Sets default values
AHuauv::AHuauv() {
	PrimaryActorTick.bCanEverTick = true;
	// Set the defualt controller
	AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.HuauvController"), NULL, LOAD_None, NULL);
	AutoPossessAI = EAutoPossessAI::PlacedInWorld;

	// This values are all pulled from the solidworks file
	this->Volume = .03554577;	
	this->CenterBuoyancy = FVector(-5.96, 0.29, -1.85); 
	this->CenterMass = FVector(-5.9, 0.46, -2.82);
	this->MassInKG = 31.02;
	this->OffsetToOrigin = FVector(-0.7, -2, 32);
}

// Sets all values that we need
void AHuauv::InitializeAgent() {
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

// Called every frame
void AHuauv::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	// Convert linear acceleration to force
	FVector linAccel = FVector(CommandArray[0], CommandArray[1], CommandArray[2]);
	linAccel = ClampVector(linAccel, -FVector(H_MAX_LIN_ACCEL), FVector(H_MAX_LIN_ACCEL));
	linAccel = ConvertLinearVector(linAccel, ClientToUE);

	// Convert angular acceleration to torque
	FVector angAccel = FVector(CommandArray[3], CommandArray[4], CommandArray[5]);
	angAccel = ClampVector(angAccel, -FVector(H_MAX_ANG_ACCEL), FVector(H_MAX_ANG_ACCEL));
	angAccel = ConvertAngularVector(angAccel, NoScale);

	RootMesh->GetBodyInstance()->AddForce(linAccel, true, true);
	RootMesh->GetBodyInstance()->AddTorqueInRadians(angAccel, true, true);
}

// For empty dynamics, damping is disabled
// Enable it when using thrusters/controller
void AHuauv::EnableDamping(){
	RootMesh->SetLinearDamping(1.0);
	RootMesh->SetAngularDamping(0.75);
}

void AHuauv::ApplyAirThrusters(float* const ThrusterArray){
	float RollTorqueToApply = FMath::Clamp(ThrusterArray[0], -AIR_MAX_ROLL, AIR_MAX_ROLL);
	float PitchTorqueToApply = FMath::Clamp(ThrusterArray[1], -AIR_MAX_PITCH, AIR_MAX_PITCH);
	float YawTorqueToApply = FMath::Clamp(ThrusterArray[2], -AIR_MAX_YAW_RATE, AIR_MAX_YAW_RATE);
	float ThrustToApply = FMath::Clamp(ThrusterArray[3], -AIR_MAX_FORCE, AIR_MAX_FORCE);

	FVector LocalThrust = FVector(0, 0, ThrustToApply);
	LocalThrust = ConvertLinearVector(LocalThrust, ClientToUE);
	FVector LocalTorque = FVector(RollTorqueToApply, PitchTorqueToApply, YawTorqueToApply);
	LocalTorque = ConvertTorque(LocalTorque, ClientToUE);

	// Apply torques and forces in global coordinates
	RootMesh->AddTorqueInRadians(GetActorRotation().RotateVector(LocalTorque));
	RootMesh->AddForce(GetActorRotation().RotateVector(LocalThrust));
}

void AHuauv::ApplyUnderwaterThrusters(float* const ThrusterArray){
	// Iterate through vertical thrusters (aerial thrusters)
	for(int i=0;i<4;i++){
		float force = FMath::Clamp(ThrusterArray[i], -H_MAX_THRUST, H_MAX_THRUST);
		FVector LocalForce = FVector(0, 0, force);
		LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

		RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
	}

	// Iterate through angled thrusters (underwater thrusters)
	for(int i=4;i<8;i++){
		float force = FMath::Clamp(ThrusterArray[i], -H_MAX_THRUST, H_MAX_THRUST);
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
