
#pragma once

#include <string>

#include "CoreTypes.h"
#include "Log.h"

namespace Engine {

    class SignalHandler {
      public:
        // Initialize signal handlers
        static void Initialize();

        // Restore default signal handlers
        static void Shutdown();

        // Check if a signal is being handled
        static bool IsHandlingSignal();

        // Get the current signal being handled
        static int GetCurrentSignal();

        // Get signal description
        static const char *GetSignalDescription(int signal);

        // Custom signal handlers
        static void SetCustomHandler(int signal, void (*handler)(int));
        static void RestoreDefaultHandler(int signal);

      private:
        // Internal signal handlers
        static void HandleSegmentationFault(int signal);
        static void HandleIllegalInstruction(int signal);
        static void HandleAbort(int signal);
        static void HandleFloatingPointException(int signal);
        static void HandleBusError(int signal);
        static void HandleInterrupt(int signal);
        static void HandleTerminate(int signal);

        // Signal state
        static thread_local bool handlingSignal;
        static thread_local int currentSignal;
    };

    // RAII signal handler installer
    class ScopedSignalHandler {
      public:
        ScopedSignalHandler() { SignalHandler::Initialize(); }

        ~ScopedSignalHandler() { SignalHandler::Shutdown(); }

      private:
        ScopedSignalHandler(const ScopedSignalHandler &) = delete;
        ScopedSignalHandler &operator=(const ScopedSignalHandler &) = delete;
    };

}  // namespace Engine
