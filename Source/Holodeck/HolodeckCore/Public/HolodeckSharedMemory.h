// Created by joshgreaves on 5/9/17.
// Documented for BiguaSim API Reference.

#pragma once

#include <string>
#include <cstring>

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#endif

/**
 * @brief A cross-platform abstraction for memory-mapped files (Shared Memory).
 * Handles the low-level OS calls to allocate, map, and release shared RAM blocks 
 * for both Windows and Linux, enabling high-speed IPC (Inter-Process Communication) 
 * between the Unreal Engine and the Python client.
 */
class HOLODECK_API HolodeckSharedMemory {
public:
    /**
     * @brief Constructs and allocates a memory-mapped file.
     * The final system path for the memory block is constructed by prepending 
     * "/HOLODECK_MEM_" and the UUID to the provided Name.
     * @param Name The specific identifier for this memory block (e.g., "auv0_ImagingSonar").
     * @param BufferSize The exact number of bytes to allocate in RAM.
     * @param UUID A unique identifier for the engine instance to prevent collisions when running multiple instances.
     */
    explicit HolodeckSharedMemory(const std::string& Name, unsigned int BufferSize, const std::string& UUID);

    /**
     * @brief Destructor.
     * Safely unmaps the memory view and closes the OS handles.
     */
    ~HolodeckSharedMemory();

    /**
     * @brief Retrieves the pointer to the start of the mapped memory block.
     * @return A void pointer to the memory buffer. Cast this to the appropriate type before writing data.
     */
    void* GetPtr() const {return MemPointer;}

    /**
     * @brief Gets the size of the allocated memory mapped file.
     * @return The size in bytes.
     */
    int Size() const {return MemSize;}

private:
    /** @brief The full OS-level path/name of the shared memory block. */
    std::string MemPath;
    
    /** @brief The size of the allocated memory in bytes. */
    unsigned int MemSize;
    
    /** @brief Pointer to the mapped memory region. */
    void* MemPointer;

    #if PLATFORM_WINDOWS
    /** @brief Windows file mapping handle. */
    HANDLE MemFile;
    #elif PLATFORM_LINUX
    /** @brief POSIX file descriptor for the shared memory object. */
    int MemFile;
    #endif

    /**
     * @brief Helper function to log fatal OS-level errors during memory allocation.
     * @param errorMessage The context of the error to print alongside the OS errno.
     */
    void LogSystemError(const std::string &errorMessage);
};