
#include <iostream>

#include "Assert.h"
#include "Events/EventSystem.h"
#include "Events/EventTypes.h"
#include "Events/EventUtils.h"

namespace Engine {
    namespace Test {

        // Custom event data for testing
        struct TestEventData : public EventData {
            int value;
            std::string message;

            TestEventData(int v, const std::string &msg)
                : value(v), message(msg) {}
        };

        // Event type for testing
        constexpr EventTypeId TEST_EVENT_TYPE =
            static_cast<EventTypeId>(CommonEventTypes::UserDefined);

        void RunEventSystemTests() {
            // Test basic subscription and publishing
            {
                bool eventReceived = false;
                int receivedValue = 0;
                std::string receivedMessage;

                // Create a subscription manager
                EventSubscriptionManager subscriptionManager;

                // Subscribe to test event
                subscriptionManager.Subscribe<TestEventData>(
                    TEST_EVENT_TYPE, [&](const TestEventData *data) {
                        eventReceived = true;
                        receivedValue = data->value;
                        receivedMessage = data->message;
                    });

                // Publish test event
                EventPublisher::PublishEvent<TestEventData>(
                    TEST_EVENT_TYPE, 42, "Hello, Events!");

                // Verify event was received
                ASSERT(eventReceived, "Event was not received");
                ASSERT(receivedValue == 42, "Incorrect value received");
                ASSERT(receivedMessage == "Hello, Events!",
                       "Incorrect message received");
            }

            // Test multiple subscribers
            {
                int callCount = 0;

                EventSubscriptionManager manager1;
                EventSubscriptionManager manager2;

                // Subscribe multiple handlers
                manager1.Subscribe<TestEventData>(
                    TEST_EVENT_TYPE,
                    [&](const TestEventData *) { callCount++; });

                manager2.Subscribe<TestEventData>(
                    TEST_EVENT_TYPE,
                    [&](const TestEventData *) { callCount++; });

                // Publish event
                EventPublisher::PublishEvent<TestEventData>(
                    TEST_EVENT_TYPE, 0, "Multiple subscribers test");

                // Verify both handlers were called
                ASSERT(callCount == 2, "Not all subscribers were notified");
            }

            // Test unsubscription
            {
                int callCount = 0;

                EventSubscriptionManager manager;

                // Subscribe to event
                manager.Subscribe<TestEventData>(
                    TEST_EVENT_TYPE,
                    [&](const TestEventData *) { callCount++; });

                // Publish event
                EventPublisher::PublishEvent<TestEventData>(
                    TEST_EVENT_TYPE, 0, "Before unsubscribe");

                // Verify handler was called
                ASSERT(callCount == 1, "Handler not called before unsubscribe");

                // Unsubscribe from event
                manager.UnsubscribeFromEvent(TEST_EVENT_TYPE);

                // Publish event again
                EventPublisher::PublishEvent<TestEventData>(
                    TEST_EVENT_TYPE, 0, "After unsubscribe");

                // Verify handler was not called again
                ASSERT(callCount == 1, "Handler called after unsubscribe");
            }

            // Test automatic unsubscription
            {
                int callCount = 0;

                {
                    EventSubscriptionManager manager;

                    // Subscribe to event
                    manager.Subscribe<TestEventData>(
                        TEST_EVENT_TYPE,
                        [&](const TestEventData *) { callCount++; });

                    // Publish event
                    EventPublisher::PublishEvent<TestEventData>(
                        TEST_EVENT_TYPE, 0, "Before destruction");

                    // Verify handler was called
                    ASSERT(callCount == 1,
                           "Handler not called before manager destruction");

                    // Manager will be destroyed here
                }

                // Publish event after manager destruction
                EventPublisher::PublishEvent<TestEventData>(
                    TEST_EVENT_TYPE, 0, "After destruction");

                // Verify handler was not called after manager destruction
                ASSERT(callCount == 1,
                       "Handler called after manager destruction");
            }

            // Test window events
            {
                bool windowResized = false;
                uint32_t newWidth = 0;
                uint32_t newHeight = 0;

                EventSubscriptionManager manager;

                // Subscribe to window resize event
                manager.Subscribe<WindowResizeEventData>(
                    static_cast<EventTypeId>(CommonEventTypes::WindowResized),
                    [&](const WindowResizeEventData *data) {
                        windowResized = true;
                        newWidth = data->width;
                        newHeight = data->height;
                    });

                // Publish window resize event
                EventPublisher::PublishEvent<WindowResizeEventData>(
                    static_cast<EventTypeId>(CommonEventTypes::WindowResized),
                    1,
                    1920,
                    1080);

                // Verify window resize event was handled
                ASSERT(windowResized, "Window resize event not received");
                ASSERT(newWidth == 1920, "Incorrect window width");
                ASSERT(newHeight == 1080, "Incorrect window height");
            }

            std::cout << "Event system tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine
