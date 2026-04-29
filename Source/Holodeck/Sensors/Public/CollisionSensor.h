// MIT License (c) 2019 BYU PCCL see LICENSE file
#pragma once

#include "Holodeck.h"

#include "Components/SceneComponent.h"
#include "HolodeckSensor.h"

#include "CollisionSensor.generated.h"

/**
 * @brief Collision detection sensor.
 * Inherits from the HolodeckSensor class.
 * Reports a boolean flag indicating whether the parent agent is currently colliding with any other physics object in the environment.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UCollisionSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    /**
     * @brief Default Constructor.
     */
    UCollisionSensor();
    
    /**
     * @brief Initializes the component and binds the collision delegates.
     */
    void InitializeComponent() override;

    /**
     * @brief Event handler for physics collisions.
     */
    UFUNCTION()
    void OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

protected:
    /** @brief Gets the number of items this sensor returns (1 boolean). */
    int GetNumItems() override { return 1; };
    
    /** @brief Gets the size of each item in bytes. */
    int GetItemSize() override { return sizeof(bool); };
    
    /** @brief Main tick function for writing data to the buffer. */
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** * @brief Pointer to whatever actor the sensor is attached to. 
     * Not owned by this class.
     */
    AActor* Parent;

    /** @brief Internal flag tracking if a collision occurred in the current frame. */
    bool bIsColliding = false;

};