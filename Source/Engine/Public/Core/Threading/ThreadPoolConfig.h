
#pragma once

#include "Core/CoreTypes.h"

namespace Engine {

    /**
     * @brief Configuration options for ThreadPool
     */
    struct ThreadPoolConfig {
        // Number of worker threads (0 = hardware concurrency)
        size_t threadCount = 0;

        // Maximum number of tasks that can be queued (0 = unlimited)
        size_t maxQueueSize = 0;

        // Whether to allow task stealing between worker threads
        bool enableWorkStealing = true;

        // Maximum time (in milliseconds) to wait for tasks during shutdown (0 =
        // wait forever)
        uint32_t shutdownTimeoutMs = 5000;

        // Whether to catch exceptions in tasks
        bool catchExceptions = true;

        // Thread name prefix for debugging
        const char* threadNamePrefix = "Worker";
    };

}  // namespace Engine
