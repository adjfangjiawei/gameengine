
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "CoreTypes.h"

namespace Engine {

    /**
     * @brief Represents an event type identifier
     */
    using EventTypeId = uint32_t;

    /**
     * @brief Base class for all event data
     */
    class EventData {
      public:
        virtual ~EventData() = default;
    };

    /**
     * @brief Represents a callback function for event handling
     */
    using EventCallback = std::function<void(const EventData *)>;

    /**
     * @brief Event system that implements a publisher-subscriber pattern
     *
     * The EventSystem allows different parts of the engine to communicate
     * through events without direct coupling. Components can subscribe to
     * specific event types and publish events that other components can react
     * to.
     */
    class EventSystem {
      public:
        /**
         * @brief Get the singleton instance of the event system
         * @return Reference to the event system instance
         */
        static EventSystem &Get();

        /**
         * @brief Subscribe to an event type
         * @param eventType The type of event to subscribe to
         * @param callback The function to call when the event occurs
         * @return A unique subscription ID that can be used to unsubscribe
         */
        uint32_t Subscribe(EventTypeId eventType, EventCallback callback);

        /**
         * @brief Unsubscribe from an event
         * @param eventType The type of event
         * @param subscriptionId The subscription ID returned from Subscribe
         */
        void Unsubscribe(EventTypeId eventType, uint32_t subscriptionId);

        /**
         * @brief Publish an event to all subscribers
         * @param eventType The type of event to publish
         * @param eventData The event data to send to subscribers
         */
        void Publish(EventTypeId eventType, const EventData *eventData);

        /**
         * @brief Clear all event subscriptions
         */
        void Clear();

      private:
        EventSystem() = default;
        ~EventSystem() = default;

        EventSystem(const EventSystem &) = delete;
        EventSystem &operator=(const EventSystem &) = delete;
        EventSystem(EventSystem &&) = delete;
        EventSystem &operator=(EventSystem &&) = delete;

      private:
        struct Subscription {
            uint32_t id;
            EventCallback callback;
        };

        std::unordered_map<EventTypeId, std::vector<Subscription>>
            m_subscribers;
        std::mutex m_mutex;
        uint32_t m_nextSubscriptionId = 1;
    };

}  // namespace Engine
