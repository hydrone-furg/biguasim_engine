// Created by joshgreaves on 5/9/17.
// Documented for BiguaSim API Reference.

#include "Holodeck.h"
#include "HolodeckServer.h"

/**
 * @brief Default constructor for the HolodeckServer.
 * Initializes the running state to false. 
 * Warning: Because this class is a UObject, it may be initialized multiple times by Unreal Engine.
 * Do not rely on singleton-qualities in the constructor; use the Start() function instead.
 */
UHolodeckServer::UHolodeckServer() {
    bIsRunning = false;
}

/**
 * @brief Destructor. 
 * Ensures the server is properly shut down and system resources are freed by calling Kill().
 */
UHolodeckServer::~UHolodeckServer() {
    Kill();
}

/**
 * @brief Initializes the server and sets up cross-process synchronization.
 * Parses the command line for a UUID to ensure unique shared memory environments.
 * Creates or opens system-level semaphores (handling both Windows and Linux implementations)
 * to lock/unlock the execution state between the Unreal Engine tick and the Python client.
 */
void UHolodeckServer::Start() {
    UE_LOG(LogHolodeck, Log, TEXT("Initializing HolodeckServer"));
    if (bIsRunning) {
        UE_LOG(LogHolodeck, Warning, TEXT("HolodeckServer is already running! Bringing it down and up"));
        Kill();
    }
    Memory = std::map<std::string, TUniquePtr<HolodeckSharedMemory>>();


    if (!FParse::Value(FCommandLine::Get(), TEXT("HolodeckUUID="), UUID))
        UUID = "";
    UE_LOG(LogHolodeck, Log, TEXT("UUID: %s"), *UUID);

#if PLATFORM_WINDOWS
    auto LoadingSemaphore = OpenSemaphore(EVENT_ALL_ACCESS, false, *(LOADING_SEMAPHORE_PATH + UUID));
    ReleaseSemaphore(LoadingSemaphore, 1, NULL);
    this->LockingSemaphore1 = CreateSemaphore(NULL, 1, 1, *(SEMAPHORE_PATH1 + UUID));
    this->LockingSemaphore2 = CreateSemaphore(NULL, 0, 1, *(SEMAPHORE_PATH2 + UUID));
#elif PLATFORM_LINUX
    auto LoadingSemaphore = sem_open(TCHAR_TO_ANSI(*(LOADING_SEMAPHORE_PATH + UUID)), O_CREAT, 0777, 0);
    if (LoadingSemaphore == SEM_FAILED) {
        LogSystemError("Unable to open loading semaphore");
    }

    LockingSemaphore1 = sem_open(TCHAR_TO_ANSI(*(SEMAPHORE_PATH1 + UUID)), O_CREAT, 0777, 1);
    if (LockingSemaphore1 == SEM_FAILED) {
        LogSystemError("Unable to open server semaphore");
    }

    LockingSemaphore2 = sem_open(TCHAR_TO_ANSI(*(SEMAPHORE_PATH2 + UUID)), O_CREAT, 0777, 0);
    if (LockingSemaphore2 == SEM_FAILED) {
        LogSystemError("Unable to open client semaphore");
    }

    int status = sem_post(LoadingSemaphore);
    if (status == -1) {
        LogSystemError("Unable to update loading semaphore");
    }

    // Client unlinks LoadingSemaphore
#endif

    bIsRunning = true;
    UE_LOG(LogHolodeck, Log, TEXT("HolodeckServer started successfully"));
}

/**
 * @brief Shuts down the server and cleans up resources.
 * Clears the shared memory map and securely closes/unlinks the cross-process semaphores.
 */
void UHolodeckServer::Kill() {
    UE_LOG(LogHolodeck, Log, TEXT("Killing HolodeckServer"));
    if (!bIsRunning) return;

    Memory.clear();

#if PLATFORM_WINDOWS
    CloseHandle(this->LockingSemaphore1);
    CloseHandle(this->LockingSemaphore2);
#elif PLATFORM_LINUX
    int status = sem_unlink(SEMAPHORE_PATH1);
    if (status == -1) {
        LogSystemError("Unable to close server semaphore");
    }

    status = sem_unlink(SEMAPHORE_PATH2);
    if (status == -1) {
        LogSystemError("Unable to close client semaphore");
    }
#endif

    bIsRunning = false;
    UE_LOG(LogHolodeck, Log, TEXT("HolodeckServer successfully shut down"));
}

/**
 * @brief Allocates or retrieves a block of shared memory.
 * If the requested key does not exist or the required buffer size has changed, 
 * it provisions a new shared memory block.
 * @param Key The unique string identifier for the memory block (e.g., a sensor's name).
 * @param BufferSize The size in bytes required for this memory block.
 * @return A void pointer to the beginning of the shared memory block.
 */
void* UHolodeckServer::Malloc(const std::string& Key, unsigned int BufferSize) {
    // If this key doesn't already exist, or the buffer size has changed, allocate the memory.
     
    if (!Memory.count(Key) || Memory[Key]->Size() != BufferSize)
    {
        UE_LOG(LogHolodeck, Log, TEXT("Mallocing %u bytes for key %s"), BufferSize, UTF8_TO_TCHAR(Key.c_str()));
        Memory[Key] = TUniquePtr<HolodeckSharedMemory>(new HolodeckSharedMemory(Key, BufferSize, TCHAR_TO_UTF8(*UUID)));
        UE_LOG(LogHolodeck, Warning, TEXT("Was not Memory[Key] MakeShared"));
    }

    return Memory[Key]->GetPtr();
}

/**
 * @brief Blocks the Unreal Engine thread until it receives a signal to proceed.
 * Waits on LockingSemaphore1, pausing the simulation tick until the Python client 
 * reads the previous state and sends new commands.
 */
void UHolodeckServer::Acquire() {
    UE_LOG(LogHolodeck, VeryVerbose, TEXT("HolodeckServer Acquiring"));
#if PLATFORM_WINDOWS
    WaitForSingleObject(this->LockingSemaphore1, INFINITE);
#elif PLATFORM_LINUX

    int status = sem_wait(LockingSemaphore1);
    if (status == -1) {
        LogSystemError("Unable to wait for server semaphore");
    }
#endif
}

/**
 * @brief Signals that Unreal Engine has finished writing the current state.
 * Releases LockingSemaphore2, waking up the waiting Python client process so it can 
 * read the new sensor data.
 */
void UHolodeckServer::Release() {
    UE_LOG(LogHolodeck, VeryVerbose, TEXT("HolodeckServer Releasing"));
#if PLATFORM_WINDOWS
    ReleaseSemaphore(this->LockingSemaphore2, 1, NULL);
#elif PLATFORM_LINUX

    int status = sem_post(LockingSemaphore2);
    if (status == -1) {
        LogSystemError("Unable to update loading semaphore");
    }
#endif
}

/**
 * @brief Checks the active status of the server.
 * @return True if the server is running and semaphores are active, false otherwise.
 */
bool UHolodeckServer::IsRunning() const {
    return bIsRunning;
}

/**
 * @brief Helper function to log critical system-level errors.
 * Primarily used to output semaphore failures along with their respective OS error codes.
 * @param errorMessage A string detailing the context of the error.
 */
void UHolodeckServer::LogSystemError(const std::string& errorMessage) {
    UE_LOG(LogHolodeck, Fatal, TEXT("%s - Error code: %d=%s"), ANSI_TO_TCHAR(errorMessage.c_str()), errno, ANSI_TO_TCHAR(strerror(errno)));
}