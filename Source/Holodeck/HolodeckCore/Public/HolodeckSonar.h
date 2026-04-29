// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSensor.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"
#include "Async/ParallelFor.h"

#include "HolodeckSonar.generated.h"

#define Pi 3.1415926535897932384626433832795
/**
 * UHolodeckSonar
 */
UCLASS(Abstract)
class HOLODECK_API UHolodeckSonar : public UHolodeckSensor
{
	GENERATED_BODY()
	
public:
	/*
	* Default Constructor
	*/
	UHolodeckSonar(){}

	/**
	* Allows parameters to be set dynamically
	*/
	virtual void ParseSensorParms(FString ParmsJson) override;

	// Agora o GetNumItems é genérico para todos!
    virtual int GetNumItems() override {
        return GetRows() * GetCols() * GetChannelCount();
    }
	
	/*
	* Cleans up octree
	*/
	virtual void BeginDestroy() override;

protected:
	//New flags for the GT sonar options
    UPROPERTY(EditAnywhere)
    bool SendGTIntensity = true;

    UPROPERTY(EditAnywhere)
    bool SendGTElevation = true;

    UPROPERTY(EditAnywhere)
    bool SendGTPointCloud = true;

    // TArray para armazenar os ponteiros das folhas que passaram pelo teste de sombra
    // Isso facilita para as classes filhas acessarem os dados brutos
    TArray<Octree*> ValidLeaves;

	void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	float RangeMax = 1000;

	UPROPERTY(EditAnywhere)
	float InitOctreeRange = 0;

	UPROPERTY(EditAnywhere)
	float RangeMin = 10;

	UPROPERTY(EditAnywhere)
	float Azimuth = 120;

	UPROPERTY(EditAnywhere)
	float Elevation = 20;

	UPROPERTY(EditAnywhere)
	int TicksPerCapture = 1;

	UPROPERTY(EditAnywhere)
	bool ViewRegion = false;

	UPROPERTY(EditAnywhere)
	int ViewOctree = -10;

	UPROPERTY(EditAnywhere)
	float ShadowEpsilon = 0;

	UPROPERTY(EditAnywhere)
	float WaterDensity = 997.0;

	UPROPERTY(EditAnywhere)
	float WaterSpeedSound = 1480.0;

	UPROPERTY(EditAnywhere)
	bool ShowWarning = true;

	//property for using approximation or not for atan2
	UPROPERTY(EditAnywhere)
	bool UseApprox = true;

	// Call at the beginning of every tick, loads octree
	void initOctree();

	// Finds all the leaves in range
	void findLeaves();

	// Shadow leaves that have been sorted
	void shadowLeaves();

	// Visualizer helpers
	void showBeam(float DeltaTime);
	virtual void showRegion(float DeltaTime);

	// Used to hold leafs when parallelized filtering happens
	TArray<TArray<Octree*>> foundLeaves;

	// Used to hold leafs when parallelized sorting/binning happens
	TArray<TArray<Octree*>> sortedLeaves;

	// Water information
	float WaterImpedance;

	// use for skipping frames
	int TickCounter = 0;

	// various computations we want to cache
	float ATan2Approx(float y, float x);
	float minAzimuth;
	float maxAzimuth;
	float minElev;
	float maxElev;

	// Métodos que as classes filhas DEVEM implementar
    virtual int GetRows() const { return 0; } 
	virtual int GetCols() const { return 0; }

    // Cálculo dinâmico do multiplicador de canais
    int GetChannelCount() const {
        int count = 1; // Canal 0 (Sempre ruidoso)
        if (SendGTIntensity) count++;
        if (SendGTElevation) count++;
        if (SendGTPointCloud) count += 3; // X, Y, Z
        return count;
    }

	virtual bool inRange(Octree* tree);
	void leavesInRange(Octree* tree, TArray<Octree*>& leafs, float stopAt);
	FVector spherToEuc(float r, float theta, float phi, FTransform SensortoWorld);

	// holds our implementation of Octrees
	Octree* octree = nullptr;
	
	// What octrees we initally make
	TArray<Octree*> toMake;
	
private:
	/*
	 * Parent
	 * After initialization, Parent contains a pointer to whatever the sensor is attached to.
	 */
	AActor* Parent;

	TArray<Octree*> agents;
	void viewLeaves(Octree* tree);

	// initialize + reserve vectors once
	TArray<Octree*> bigLeaves;

	// various computations we want to cache
	float sqrt3_2;
	float sinOffset;
};
