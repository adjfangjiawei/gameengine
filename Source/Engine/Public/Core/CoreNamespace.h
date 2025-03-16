#pragma once

/**
 * @brief Core engine namespace definitions
 *
 * This file defines the standard namespace structure used throughout the
 * engine. All code should use these namespaces for consistency.
 */

// Main engine namespace
namespace Engine {

    // Memory subsystem namespace
    namespace Memory {
        // Memory management code goes here
    }

    // Other subsystem namespaces
    namespace Math {}
    namespace Logging {}
    namespace FileSystem {}
    namespace Events {}
    namespace Threading {}
    namespace Reflection {}
    // etc.

}  // namespace Engine

// Legacy namespace for backward compatibility - will be deprecated
// Replace any Engine namespace usage with Engine namespace
namespace Engine = Engine;