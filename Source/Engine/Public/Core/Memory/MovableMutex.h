#pragma once

#include <memory>
#include <mutex>

namespace Engine {
    namespace Memory {

        /**
         * A wrapper around std::mutex that can be moved.
         * Standard mutex cannot be copied or moved, but this wrapper allows for
         * moving by using an indirection through std::unique_ptr.
         */
        class MovableMutex {
          private:
            std::unique_ptr<std::mutex> mtx;

          public:
            MovableMutex() : mtx(std::make_unique<std::mutex>()) {}
            MovableMutex(MovableMutex &&) = default;
            MovableMutex &operator=(MovableMutex &&) = default;

            // Deleted copy operations
            MovableMutex(const MovableMutex &) = delete;
            MovableMutex &operator=(const MovableMutex &) = delete;

            // Explicit conversion to std::mutex reference (not implicit)
            std::mutex &get_mutex() { return *mtx; }
            const std::mutex &get_mutex() const { return *mtx; }

            void lock() { mtx->lock(); }
            void unlock() { mtx->unlock(); }
            bool try_lock() { return mtx->try_lock(); }

            // Allow const version of lock/unlock as well
            void lock() const { const_cast<std::mutex *>(mtx.get())->lock(); }
            void unlock() const {
                const_cast<std::mutex *>(mtx.get())->unlock();
            }
            bool try_lock() const {
                return const_cast<std::mutex *>(mtx.get())->try_lock();
            }
        };

    }  // namespace Memory
}  // namespace Engine
