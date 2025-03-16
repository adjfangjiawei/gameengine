
#pragma once

#include <type_traits>
#include <vector>

#include "Core/Events/EventSystem.h"

namespace Engine {

    /**
     * @brief Helper class for managing event subscriptions
     *
     * This class automatically unsubscribes from all events when destroyed,
     * helping prevent memory leaks and dangling subscriptions.
     */
    class EventSubscriptionManager {
      public:
        EventSubscriptionManager() = default;
        ~EventSubscriptionManager() { UnsubscribeAll(); }

        // Prevent copying
        EventSubscriptionManager(const EventSubscriptionManager &) = delete;
        EventSubscriptionManager &operator=(const EventSubscriptionManager &) =
            delete;

        // Allow moving
        EventSubscriptionManager(EventSubscriptionManager &&other) noexcept
            : m_subscriptions(std::move(other.m_subscriptions)) {
            other.m_subscriptions.clear();
        }

        EventSubscriptionManager &operator=(
            EventSubscriptionManager &&other) noexcept {
            if (this != &other) {
                UnsubscribeAll();
                m_subscriptions = std::move(other.m_subscriptions);
                other.m_subscriptions.clear();
            }
            return *this;
        }

        /**
         * @brief Subscribe to an event with automatic type checking
         * @tparam T The event data type
         * @param eventType The type of event to subscribe to
         * @param callback The callback function to handle the event
         */
        template <typename T>
        void Subscribe(EventTypeId eventType,
                       std::function<void(const T *)> callback) {
            static_assert(std::is_base_of<EventData, T>::value,
                          "Event data type must derive from EventData");

            // Store callback in a shared_ptr to manage its lifetime
            auto callbackPtr = std::make_shared<std::function<void(const T *)>>(
                std::move(callback));

            auto wrapper = [callbackPtr](const EventData *data) {
                const T *typedData = static_cast<const T *>(data);
                (*callbackPtr)(typedData);
            };

            uint32_t id = EventSystem::Get().Subscribe(eventType, wrapper);
            m_subscriptions.push_back({eventType, id});
        }

        /**
         * @brief Unsubscribe from a specific event type
         * @param eventType The event type to unsubscribe from
         */
        void UnsubscribeFromEvent(EventTypeId eventType) {
            auto it = std::remove_if(m_subscriptions.begin(),
                                     m_subscriptions.end(),
                                     [eventType](const Subscription &sub) {
                                         if (sub.eventType == eventType) {
                                             EventSystem::Get().Unsubscribe(
                                                 sub.eventType, sub.id);
                                             return true;
                                         }
                                         return false;
                                     });
            m_subscriptions.erase(it, m_subscriptions.end());
        }

        /**
         * @brief Unsubscribe from all events
         */
        void UnsubscribeAll() {
            for (const auto &sub : m_subscriptions) {
                EventSystem::Get().Unsubscribe(sub.eventType, sub.id);
            }
            m_subscriptions.clear();
        }

      private:
        struct Subscription {
            EventTypeId eventType;
            uint32_t id;
        };

        std::vector<Subscription> m_subscriptions;
    };

    /**
     * @brief Helper functions for type-safe event publishing
     */
    class EventPublisher {
      public:
        /**
         * @brief Publish an event with type checking
         * @tparam T The event data type
         * @param eventType The type of event to publish
         * @param eventData The event data to publish
         */
        template <typename T>
        static void Publish(EventTypeId eventType, const T &eventData) {
            static_assert(std::is_base_of<EventData, T>::value,
                          "Event data type must derive from EventData");
            EventSystem::Get().Publish(eventType, &eventData);
        }

        /**
         * @brief Create and publish an event in one step
         * @tparam T The event data type
         * @tparam Args The argument types for constructing the event data
         * @param eventType The type of event to publish
         * @param args The arguments to construct the event data
         */
        template <typename T, typename... Args>
        static void PublishEvent(EventTypeId eventType, Args &&...args) {
            static_assert(std::is_base_of<EventData, T>::value,
                          "Event data type must derive from EventData");
            T eventData(std::forward<Args>(args)...);
            Publish(eventType, eventData);
        }
    };

}  // namespace Engine
