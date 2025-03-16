#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "Assert.h"
#include "CoreTypes.h"

namespace Engine {

    /**
     * @brief Interface for type erasure in pool storage
     */
    class IObjectPool {
      public:
        virtual ~IObjectPool() = default;
    };

    // Forward declaration
    class ObjectPoolManager;

    /**
     * @brief A thread-safe object pool for efficient object reuse
     *
     * ObjectPool maintains a pool of pre-allocated objects that can be
     * reused to avoid expensive allocation and deallocation operations.
     * It is particularly useful for frequently created and destroyed objects.
     *
     * @tparam T The type of objects to pool
     */
    template <typename T>
    class ObjectPool : public IObjectPool {
      public:
        /**
         * @brief Constructor
         * @param initialSize Initial number of objects to pre-allocate
         * @param growSize Number of objects to allocate when pool is empty
         * @param maxSize Maximum number of objects the pool can hold (0 for
         * unlimited)
         */
        explicit ObjectPool(size_t initialSize = 32,
                            size_t growSize = 32,
                            size_t maxSize = 0)
            : m_growSize(growSize), m_maxSize(maxSize) {
            Grow(initialSize);
        }

        /**
         * @brief Destructor
         */
        ~ObjectPool() { Clear(); }

        // Prevent copying
        ObjectPool(const ObjectPool &) = delete;
        ObjectPool &operator=(const ObjectPool &) = delete;

        /**
         * @brief Acquire an object from the pool
         * @return Unique pointer to the object
         */
        std::unique_ptr<T, std::function<void(T *)>> Acquire() {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_available.empty()) {
                if (m_maxSize == 0 || m_objects.size() < m_maxSize) {
                    Grow(m_growSize);
                } else {
                    return nullptr;
                }
            }

            T *obj = m_available.front();
            m_available.pop();

            // Create deleter that returns object to pool
            auto deleter = [this](T *ptr) { ReturnToPool(ptr); };

            return std::unique_ptr<T, std::function<void(T *)>>(obj, deleter);
        }

        /**
         * @brief Get the number of available objects in the pool
         * @return Number of available objects
         */
        size_t GetAvailableCount() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_available.size();
        }

        /**
         * @brief Get the total number of objects managed by the pool
         * @return Total number of objects
         */
        size_t GetTotalSize() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_objects.size();
        }

        /**
         * @brief Clear all objects from the pool
         */
        void Clear() {
            std::lock_guard<std::mutex> lock(m_mutex);

            while (!m_available.empty()) {
                m_available.pop();
            }

            for (T *obj : m_objects) {
                delete obj;
            }

            m_objects.clear();
        }

        /**
         * @brief Set a function to be called when initializing new objects
         * @param func Initialization function
         */
        void SetInitFunction(std::function<void(T *)> func) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_initFunc = func;
        }

        /**
         * @brief Set a function to be called when resetting objects
         * @param func Reset function
         */
        void SetResetFunction(std::function<void(T *)> func) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_resetFunc = func;
        }

      private:
        /**
         * @brief Grow the pool by creating new objects
         * @param count Number of objects to create
         */
        void Grow(size_t count) {
            for (size_t i = 0; i < count; ++i) {
                T *obj = new T();

                if (m_initFunc) {
                    m_initFunc(obj);
                }

                m_objects.push_back(obj);
                m_available.push(obj);
            }
        }

        /**
         * @brief Return an object to the pool
         * @param obj Object to return
         */
        void ReturnToPool(T *obj) {
            if (obj == nullptr) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_resetFunc) {
                m_resetFunc(obj);
            }

            m_available.push(obj);
        }

      private:
        std::vector<T *> m_objects;
        std::queue<T *> m_available;
        mutable std::mutex m_mutex;

        const size_t m_growSize;
        const size_t m_maxSize;

        std::function<void(T *)> m_initFunc;
        std::function<void(T *)> m_resetFunc;
    };

    /**
     * @brief Helper class for managing object pools
     *
     * This class provides a centralized way to access and manage multiple
     * object pools.
     */
    class ObjectPoolManager {
      public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the object pool manager
         */
        static ObjectPoolManager &Get() {
            static ObjectPoolManager instance;
            return instance;
        }

        /**
         * @brief Get or create an object pool for a specific type
         * @tparam T The type of objects in the pool
         * @param initialSize Initial pool size
         * @param growSize Growth size when pool is empty
         * @param maxSize Maximum pool size (0 for unlimited)
         * @return Reference to the object pool
         */
        template <typename T>
        ObjectPool<T> &GetPool(size_t initialSize = 32,
                               size_t growSize = 32,
                               size_t maxSize = 0) {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Use type name as key
            const char *typeName = typeid(T).name();

            auto it = m_pools.find(typeName);
            if (it == m_pools.end()) {
                // Create new pool
                auto pool = std::make_unique<ObjectPool<T>>(
                    initialSize, growSize, maxSize);

                auto result = m_pools.emplace(
                    typeName, std::unique_ptr<IObjectPool>(pool.release()));

                it = result.first;
            }

            return *static_cast<ObjectPool<T> *>(it->second.get());
        }

      private:
        ObjectPoolManager() = default;
        ~ObjectPoolManager() = default;

        // Prevent copying and moving
        ObjectPoolManager(const ObjectPoolManager &) = delete;
        ObjectPoolManager &operator=(const ObjectPoolManager &) = delete;
        ObjectPoolManager(ObjectPoolManager &&) = delete;
        ObjectPoolManager &operator=(ObjectPoolManager &&) = delete;

      private:
        std::unordered_map<std::string, std::unique_ptr<IObjectPool>> m_pools;
        std::mutex m_mutex;
    };

}  // namespace Engine
