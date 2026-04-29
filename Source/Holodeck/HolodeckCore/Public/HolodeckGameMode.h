// Written by joshgreaves.
// Documented for BiguaSim API Reference.

#pragma once

#include "Holodeck.h"
#include "Octree.h"
#include "GameFramework/GameMode.h"
#include "HolodeckGameInstance.h"
#include "CommandCenter.h"
#include "HolodeckGameMode.generated.h"

/**
 * @brief The base game mode for BiguaSim.
 * HolodeckGameModeBP can be used to turn on and off core simulation functionality
 * directly from the Unreal Engine editor.
 */
UCLASS()
class AHolodeckGameMode : public AGameMode
{
    GENERATED_BODY()
    
public:
    /**
      * @brief Default Constructor.
      * @param ObjectInitializer Internal Unreal Engine object initializer.
      */
    explicit AHolodeckGameMode(const FObjectInitializer& ObjectInitializer);

    /**
      * @brief Ticks the game mode.
      * Checks for changes in settings, handles reset signals, and ticks the game instance and command center.
      * @param DeltaSeconds How many seconds passed since the last tick.
      */
    void Tick(float DeltaSeconds) override;

    /**
      * @brief Called when the game begins or when spawned.
      * Registers all server settings, initializes the Octree, and limits the framerate if requested.
      */
    void StartPlay() override;

    /** @brief Master switch to turn off simulation communication functionality. */
    UPROPERTY(EditAnywhere)
    bool bHolodeckIsOn;

    /**
      * @brief Returns the private server pointer that the game instance contains.
      * @return Pointer to the active UHolodeckServer.
      */
    UHolodeckServer* GetAssociatedServer() { return this->Server; };

    // =========================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS (Called from C++, executed in UE Blueprints)
    // =========================================================================

    /**
     * @brief Spawns a new simulation agent into the world.
     * @param Type The class type or blueprint name of the agent.
     * @param Location The world coordinates to spawn the agent.
     * @param Rotation The initial orientation of the agent.
     * @param Name The unique identifier name for the spawned agent.
     * @param IsMainAgent True if this agent is the primary focus of the simulation.
     * @return Pointer to the spawned AHolodeckAgent.
     */
    UFUNCTION(BlueprintImplementableEvent)
    AHolodeckAgent* SpawnAgent(const FString& Type, const FVector& Location, const FRotator& Rotation, const FString& Name, bool IsMainAgent);
    
    /**
     * @brief Teleports the main viewport or spectator camera to a specific location.
     * @param Location Target world coordinates.
     * @param Rotation Target camera rotation (pitch, yaw, roll).
     */
    UFUNCTION(BlueprintImplementableEvent)
    void TeleportCamera(const FVector& Location, const FVector& Rotation);
    
    /**
     * @brief Executes a custom command sent from the Python client.
     * @param Name The string identifier of the custom command.
     * @param NumberParameters Array of float arguments.
     * @param StringParameters Array of string arguments.
     */
    UFUNCTION(BlueprintImplementableEvent)
    void ExecuteCustomCommand(const FString& Name, const TArray<float>& NumberParameters, const TArray<FString>& StringParameters);
    
    /**
     * @brief Searches the current level for an actor matching a specific tag.
     * @param Tag The exact string tag to search for.
     * @return Pointer to the first AActor found with the given tag.
     */
    UFUNCTION(BlueprintImplementableEvent)
    AActor* FindActorWithTag(const FString& Tag);
    
    /**
     * @brief Retrieves a custom numerical setting from the world Blueprint.
     * @param Key The name of the parameter.
     * @return The float value associated with the key.
     */
    UFUNCTION(BlueprintImplementableEvent)
    float GetWorldNum(const FString& Key);
    
    /**
     * @brief Retrieves a custom string setting from the world Blueprint.
     * @param Key The name of the parameter.
     * @return The string value associated with the key.
     */
    UFUNCTION(BlueprintImplementableEvent)
    FString GetWorldString(const FString& Key);
    
    /**
     * @brief Retrieves a custom boolean setting from the world Blueprint.
     * @param Key The name of the parameter.
     * @return The boolean value associated with the key.
     */
    UFUNCTION(BlueprintImplementableEvent)
    bool GetWorldBool(const FString& Key);

    /**
     * @brief Logs a critical error message to the engine console and stops execution.
     * @param Message The error message to log.
     */
    UFUNCTION(BlueprintCallable)
    void LogFatalMessage(const FString& Message);

private:
    /**
      * @brief Registers all the internal settings and buffers on the shared memory server.
      */
    void RegisterSettings();

    /** @brief Shared memory buffer pointer to check if the Python client requested a level reset. */
    bool* ResetSignal;

    /** @brief The communication server managing shared memory/sockets. */
    UPROPERTY()
    UHolodeckServer* Server;
    
    /** @brief The core game instance holding simulation state. */
    UPROPERTY()
    UHolodeckGameInstance* Instance;
    
    /** @brief The command center responsible for parsing Python client commands. */
    UPROPERTY()
    UCommandCenter* CommandCenter;
};