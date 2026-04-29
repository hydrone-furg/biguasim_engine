#include "Holodeck.h"
#include "BlueBoatControlPD.h"


UBlueBoatControlPD::UBlueBoatControlPD(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer), PositionController(BB_POS_P, 0, BB_POS_D), YawController(BB_YAW_P, 0, BB_YAW_D) {}

void UBlueBoatControlPD::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (BlueBoat == nullptr) {
		BlueBoat = static_cast<ABlueBoat*>(BlueBoatController->GetPawn());
		if (BlueBoat == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("UBlueBoatControlPD couldn't get BlueBoat reference"));
			return;
		}
		
		BlueBoat->EnableDamping();
		d = UKismetMathLibrary::Abs(BlueBoat->thrusterLocations[0].Y) / 100;
	}

	// Apply gravity & buoyancy
	BlueBoat->ApplyBuoyantForce();

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	// ALL calculations here are done in HoloOcean frame & units. 

	// Get current position, velocity, yaw, and yaw rate
	FVector Position = BlueBoat->RootMesh->GetCenterOfMass();
	Position = ConvertLinearVector(Position, UEToClient);
	

	FVector LinearVelocity = BlueBoat->RootMesh->GetPhysicsLinearVelocity();
	LinearVelocity = ConvertLinearVector(LinearVelocity, UEToClient);

	float Yaw = RotatorToRPY(BlueBoat->GetActorRotation()).Z;

	FVector AngularVelocity = BlueBoat->RootMesh->GetPhysicsAngularVelocityInDegrees();
	AngularVelocity = ConvertAngularVector(AngularVelocity, NoScale);
	float YawRate = AngularVelocity.Z;
	
	// Get desired information
	FVector DesiredPosition = FVector(InputCommandFloat[0], InputCommandFloat[1], 0);
	float DesiredYaw = UKismetMathLibrary::Atan2(Position.Y-DesiredPosition.Y, Position.X-DesiredPosition.X);
	DesiredYaw = FMath::RadiansToDegrees(DesiredYaw) + 180.0;

	// Compute force & torque to apply
	float tau = YawController.ComputePIDDirect(DesiredYaw, Yaw, YawRate, DeltaSeconds, true, true);
	float f = PositionController.ComputePIDDirect(FVector::Dist2D(DesiredPosition, Position), 0, 0, DeltaSeconds);
	UE_LOG(LogHolodeck, Warning, TEXT("f %f, t %f"), f, tau);

	// Clamp
	tau = FMath::Clamp(tau, -BB_CONTROL_MAX_TORQUE, BB_CONTROL_MAX_TORQUE);
	f = FMath::Clamp(f, -BB_CONTROL_MAX_FORCE, BB_CONTROL_MAX_FORCE);

	// Convert to left & right forces & send to thrusters
	float ThrusterCommands[2];
	ThrusterCommands[0] = f/2 - tau / (2*d);
	ThrusterCommands[1] = f/2 + tau / (2*d);
	BlueBoat->ApplyThrusters(ThrusterCommands);

	// Empty out physics controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}