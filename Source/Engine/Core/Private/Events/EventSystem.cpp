
#include "Events/EventSystem.h"

#include <algorithm>

#include "Assert.h"

namespace Engine {

    EventSystem &EventSystem::Get() {
        static EventSystem instance;
        return instance;
    }

    uint32_t EventSystem::Subscribe(EventTypeId eventType,
                                    EventCallback callback) {
        ASSERT(callback != nullptr, "Event callback cannot be null");

        std::lock_guard<std::mutex> lock(m_mutex);

        uint32_t subscriptionId = m_nextSubscriptionId++;

        // Create subscription
        Subscription subscription{subscriptionId, std::move(callback)};

        // Add to subscribers
        m_subscribers[eventType].push_back(std::move(subscription));

        return subscriptionId;
    }

    void EventSystem::Unsubscribe(EventTypeId eventType,
                                  uint32_t subscriptionId) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_subscribers.find(eventType);
        if (it != m_subscribers.end()) {
            auto &subscriptions = it->second;
            auto subIt =
                std::find_if(subscriptions.begin(),
                             subscriptions.end(),
                             [subscriptionId](const Subscription &sub) {
                                 return sub.id == subscriptionId;
                             });

            if (subIt != subscriptions.end()) {
                subscriptions.erase(subIt);

                // If no more subscribers for this event type, remove the event
                // type entry
                if (subscriptions.empty()) {
                    m_subscribers.erase(it);
                }
            }
        }
    }

    void EventSystem::Publish(EventTypeId eventType,
                              const EventData *eventData) {
        ASSERT(eventData != nullptr, "Event data cannot be null");

        // Create a copy of the subscribers list to allow for safe iteration
        std::vector<Subscription> subscribers;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_subscribers.find(eventType);
            if (it != m_subscribers.end()) {
                subscribers = it->second;
            }
        }

        // Notify all subscribers
        for (const auto &subscription : subscribers) {
            subscription.callback(eventData);
        }
    }

    void EventSystem::Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_subscribers.clear();
        m_nextSubscriptionId = 1;
    }

}  // namespace Engine
