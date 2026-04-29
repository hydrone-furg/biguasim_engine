// Created by joshgreaves on 5/9/17.
// Documented for BiguaSim API Reference.

#pragma once

#include "Holodeck.h"

#include <map>
#include <memory>
#include <string>
#include <cstring>

#include "HolodeckSharedMemory.h"
#if PLATFORM_WINDOWS
#define LOADING_SEMAPHORE_PATH "Global\\HOLODECK_LOADING_SEM"
#define SEMAPHORE_PATH1 "Global\\HOLODECK_SEMAPHORE_SERVER"
#define SEMAPHORE_PATH2 "Global\\HOLODECK_SEMAPHORE_CLIENT"
#include "AllowWindowsPlatformTypes.h"
#include "Windows.h"
#include "HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
#define LOADING_SEMAPHORE_PATH "/HOLODECK_LOADING_SEM"
#define SEMAPHORE_PATH1 "/HOLODECK_SEMAPHORE_SERVER"
#define SEMAPHORE_PATH2 "/HOLODECK_SEMAPHORE_CLIENT"
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <cerrno>

#endif

#include "HolodeckServer.generated.h"

/* Forward declare HolodeckAgent Class*/
class AHolodeckAgent;

/**
 * @brief Core communication server for BiguaSim.
 * This class resides in the engine and handles the passing of messages to the Python client
 * through shared memory. A new shared memory block is created for each sensor, action
 * space, and setting. 
 * * There should only be one UHolodeckServer active at a time, instantiated by the
 * HolodeckGameInstance.
 */
UCLASS()
class HOLODECK_API UHolodeckServer : public UObject {
    GENERATED_BODY()

public:

    /**
     * @brief Default Constructor.
     */
    UHolodeckServer();

    /**
     * @brief Default Destructor.
     */
    ~UHolodeckServer();

    /**
     * @brief Starts the server.
     * Initializes the IPC semaphores based on the current platform (Windows or Linux).
     */
    void Start();

    /**
     * @brief Shuts the server down.
     * Closes the system semaphores and clears the shared memory map.
     */
    void Kill();

    /**
     * @brief Allocates or retrieves a shared memory block.
     * If memory has already been malloc'ed with the same key, the block is overwritten.
     * @param Key The unique string key for this block of memory.
     * @param BufferSize The size to allocate in bytes.
     * @return A pointer to the start of the assigned shared memory.
     */
    void* Malloc(const std::string& Key, unsigned int BufferSize);

    /**
     * @brief Acquires the mutex to allow the next tick to occur. 
     * Will block the Unreal Engine thread until the Python client releases the lock.
     */
    void Acquire();

    /**
     * @brief Releases the mutex to allow the Python client to tick.
     * Signals the client that Unreal Engine has finished updating the shared memory.
     */
    void Release();

    /**
     * @brief Gets whether the server is currently running.
     * @return True if the server and semaphores are active.
     */
    bool IsRunning() const;

    /**
     * @brief Generates a unique shared memory key for a specific item belonging to an agent.
     * @param AgentName The name of the agent (std::string).
     * @param ItemName The name of the item, like a sensor (std::string).
     * @return A formatted key string ("AgentName_ItemName").
     */
    static const std::string MakeKey(const std::string& AgentName, const std::string& ItemName) {
        return AgentName + "_" + ItemName;
    }

    /**
     * @brief Generates a unique shared memory key for a specific item belonging to an agent.
     * @param AgentName The name of the agent (FString).
     * @param ItemName The name of the item (std::string).
     * @return A formatted key string ("AgentName_ItemName").
     */
    static const std::string MakeKey(const FString& AgentName, const std::string& ItemName) {
        return std::string(TCHAR_TO_UTF8(*AgentName)) + "_" + ItemName;
    }

    /**
     * @brief Generates a unique shared memory key for a specific item belonging to an agent.
     * @param AgentName The name of the agent (FString).
     * @param ItemName The name of the item (FString).
     * @return A formatted key string ("AgentName_ItemName").
     */
    static const std::string MakeKey(const FString& AgentName, const FString& ItemName) {
        return std::string(TCHAR_TO_UTF8(*AgentName)) + "_" + std::string(TCHAR_TO_UTF8(*ItemName));
    }

    /** @brief Stores pointers to all the active agents within the world map. */
    TMap<FString, AHolodeckAgent*> AgentMap;

private:

    /** @brief Unique Identifier for this server instance, passed via command line. */
    FString UUID;
    
    /** @brief Maps string keys to their respective shared memory block pointers. */
    std::map<std::string, TUniquePtr<HolodeckSharedMemory>> Memory;
    
    /** @brief Internal flag tracking if the server is currently active. */
    bool bIsRunning;

    #if PLATFORM_WINDOWS
    /** @brief Windows semaphore used to pause the Unreal Engine tick. */
    HANDLE LockingSemaphore1;
    /** @brief Windows semaphore used to wake up the Python client. */
    HANDLE LockingSemaphore2;
    #elif PLATFORM_LINUX
    /** @brief POSIX semaphore used to pause the Unreal Engine tick. */
    sem_t* LockingSemaphore1;
    /** @brief POSIX semaphore used to wake up the Python client. */
    sem_t* LockingSemaphore2;
    #endif

    /**
     * @brief Helper to log fatal OS-level errors during semaphore creation/destruction.
     * @param errorMessage The context of the error to print alongside the OS errno.
     */
    void LogSystemError(const std::string &errorMessage);
};