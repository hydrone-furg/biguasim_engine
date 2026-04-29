// Written by joshgreaves.
// Documented for BiguaSim API Reference.

#include "Holodeck.h"
#include "HolodeckGameMode.h"

const char RESET_KEY[] = "RESET";
const int RESET_BYTES = 1;

/**
 * @brief Default constructor for the GameMode.
 * Initializes the game mode, enabling actor ticking and setting the tick group to PrePhysics
 * so the simulation logic runs before the physics engine steps.
 * * @param ObjectInitializer Internal Unreal Engine object initializer.
 */
AHolodeckGameMode::AHolodeckGameMode(const FObjectInitializer& ObjectInitializer) : AGameMode(ObjectInitializer), bHolodeckIsOn(true) {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    UE_LOG(LogHolodeck, Log, TEXT("HolodeckGameMode initialized"));
}

/**
 * @brief Called every frame to update the simulation state.
 * Propagates the tick to the GameInstance and CommandCenter. It also listens for the 
 * shared memory ResetSignal; if triggered by the Python client, it reloads the current level.
 * * @param DeltaSeconds The time in seconds since the last tick.
 */
void AHolodeckGameMode::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    // If !bHolodeckIsOn, then we never got instance or reset signal,
    // so we don't need to check bOn here.
    if (this->Instance)
        this->Instance->Tick(DeltaSeconds);
    if (this->CommandCenter)
        this->CommandCenter->Tick(DeltaSeconds);
        
    // Check if we should reset, and then reset the level. 
    if (ResetSignal && *ResetSignal) {
        UGameplayStatics::OpenLevel(this->Instance, FName(*GetWorld()->GetName()), false);
        *ResetSignal = false;
    }
}

/**
 * @brief Executed when the game starts or when spawned.
 * Handles the core initialization of the simulation environment:
 * - Initializes the acoustic Octree for sensors.
 * - Caps the framerate if the "FramesPerSec" command line argument is provided.
 * - Starts the Server for shared memory communication.
 * - Initializes the CommandCenter to parse Python commands.
 */
void AHolodeckGameMode::StartPlay() {
    UE_LOG(LogHolodeck, Log, TEXT("HolodeckGameMode starting play"));

    // To prevent crashing in standalone games, check the HolodeckOn command is supplied.
    // This overrides the bHolodeckIsOn value supplied in the editor.
    //if (GetWorld()->WorldType == EWorldType::Game)
    //  bHolodeckIsOn = FParse::Param(FCommandLine::Get(), TEXT("HolodeckOn"));

    // Make sure Octree is properly initialized
    Octree::initOctree(GetWorld());

    // Cap our tickrate
    int FramesPerSec;
    if (FParse::Value(FCommandLine::Get(), TEXT("FramesPerSec="), FramesPerSec)) {
        FString command = "t.MaxFPS " + FString::FromInt(FramesPerSec);
        bool succeeded = GEngine->Exec(GetWorld(), *command);

        if (!succeeded) {
            UE_LOG(LogHolodeck, Warning, TEXT("Unable to cap frametrate"));
        }
    }

    if (bHolodeckIsOn) {
        this->Instance = (UHolodeckGameInstance*)(GetGameInstance());
        if (this->Instance) {
            this->Instance->StartServer();
            Server = this->Instance->GetServer();

            RegisterSettings();
        } else {
            UE_LOG(LogHolodeck, Warning, TEXT("Game Instance couldn't be found and initialized"));
        }
        if (this->Server) {
            this->CommandCenter = NewObject<UCommandCenter>();
            CommandCenter->Init(Server, this);
        }
    }

    Super::StartPlay();
}

/**
 * @brief Allocates shared memory blocks for global engine settings.
 * Specifically allocates the memory required for the Python client to trigger a level reset.
 */
void AHolodeckGameMode::RegisterSettings() {
    UE_LOG(LogHolodeck, Log, TEXT("Registering Settings"));
    if (Server != nullptr) {
        ResetSignal = static_cast<bool*>(Server->Malloc(RESET_KEY, RESET_BYTES));
        UE_LOG(LogHolodeck, Log, TEXT("Reset signal registered"));
    }
}

/**
 * @brief Logs a critical error message and terminates the engine process.
 * * @param Message The string message detailing the fatal error.
 */
void AHolodeckGameMode::LogFatalMessage(const FString& Message) {
    UE_LOG(LogHolodeck, Fatal, TEXT("%s"), *Message);
}