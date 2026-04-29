// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "CollisionSensor.h"

/**
 * @brief Default Constructor.
 * Sets the sensor to tick every frame and requests component initialization.
 */
UCollisionSensor::UCollisionSensor() {
    PrimaryComponentTick.bCanEverTick = true;
    bWantsInitializeComponent = true;
    SensorName = "CollisionSensor";
}

/**
 * @brief Called on level load, before BeginPlay.
 * It's important for these components to be initialized before the actor's BeginPlay() is called.
 * Sets up the hit delegate and binds it to the parent actor to listen for physics collisions.
 */
void UCollisionSensor::InitializeComponent() {
    Super::InitializeComponent();

    Parent = this->GetOwner();
    
    // Set up the hit delegate, then give it to the parent. 
    // The parent will then call OnHit whenever it collides. 
    FScriptDelegate HitDelegate;
    HitDelegate.BindUFunction(this, TEXT("OnHit"));
    if (Parent) {
        Parent->OnActorHit.AddUnique(HitDelegate);
    }
    else {
        UE_LOG(LogHolodeck, Fatal, TEXT("UCollisionSensor::Parent was never initialized. Cannot add HitDelegate"));
    }
}

/**
 * @brief Updates the sensor data buffer every frame.
 * Writes the current collision status to the shared memory buffer, then resets the colliding boolean to false for the next frame.
 * @param DeltaTime The time elapsed since the last tick.
 * @param TickType The type of tick this is.
 * @param ThisTickFunction Tick function that is calling this.
 */
void UCollisionSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    // Update the buffer to current collision status, then set the colliding bool to false. 
    if (Parent != nullptr && bOn) {
        bool* BoolBuffer = static_cast<bool*>(Buffer);
        BoolBuffer[0] = bIsColliding;
    }
    bIsColliding = false;
}

/**
 * @brief Callback function triggered when the parent actor hits a solid object.
 * @param SelfActor The actor this component is attached to.
 * @param OtherActor The actor that was hit.
 * @param NormalImpulse The force of the collision.
 * @param Hit Detailed information about the physics collision.
 */
void UCollisionSensor::OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit) {
    bIsColliding = true;
}