
#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "CoreTypes.h"

namespace Engine {

    // Forward declarations
    class Timer;
    class TimeZone;
    class TimeRange;

    class Time {
      public:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;
        using Duration = std::chrono::duration<double>;
        using TimerCallback = std::function<void()>;

        // Time format constants
        static constexpr const char* FORMAT_ISO8601 = "%Y-%m-%dT%H:%M:%SZ";
        static constexpr const char* FORMAT_DATE = "%Y-%m-%d";
        static constexpr const char* FORMAT_TIME = "%H:%M:%S";
        static constexpr const char* FORMAT_DATETIME = "%Y-%m-%d %H:%M:%S";
        static constexpr const char* FORMAT_COMPACT = "%Y%m%d%H%M%S";

        // Initialize the time system
        static void Initialize();

        // Update the time system (call this once per frame)
        static void Update();

        // Get time since engine start in seconds
        static double GetTime();

        // Get time since last frame in seconds
        static double GetDeltaTime();

        // Get unscaled delta time (not affected by time scale)
        static double GetUnscaledDeltaTime();

        // Get the current time scale
        static double GetTimeScale();

        // Set the time scale (1.0 = normal, 0.5 = half speed, 2.0 = double
        // speed)
        static void SetTimeScale(double scale);

        // Get fixed time step for physics and other fixed-time systems
        static double GetFixedTimeStep();

        // Set fixed time step
        static void SetFixedTimeStep(double step);

        // Get total number of frames since start
        static uint64_t GetFrameCount();

        // Get frames per second
        static double GetFPS();

        // Get time since last fixed update
        static double GetFixedTime();

        // Check if it's time for a fixed update
        static bool IsTimeForFixedUpdate();

        // Get system time as string (format: YYYY-MM-DD HH:MM:SS)
        static std::string GetSystemTimeString();

        // Format time using custom format string
        static std::string FormatTime(const Time::TimePoint& time,
                                      const char* format);
        static std::string FormatTime(const Time::TimePoint& time,
                                      const std::string& format);

        // Parse time from string with error handling
        static bool ParseTime(const std::string& timeStr,
                              const char* format,
                              TimePoint& outTime);
        static bool ParseTime(const std::string& timeStr,
                              const std::string& format,
                              TimePoint& outTime);

        // Timer management
        static Timer* CreateTimer(const TimerCallback& callback,
                                  double interval,
                                  bool repeat = false);
        static void DestroyTimer(Timer* timer);
        static void PauseTimer(Timer* timer);
        static void ResumeTimer(Timer* timer);

        // Date operations
        static Time::TimePoint AddDays(const Time::TimePoint& time, int days);
        static Time::TimePoint AddMonths(const Time::TimePoint& time,
                                         int months);
        static Time::TimePoint AddYears(const Time::TimePoint& time, int years);
        static int DaysBetween(const Time::TimePoint& t1,
                               const Time::TimePoint& t2);

        // Time zone operations
        static Time::TimePoint ToLocalTime(const Time::TimePoint& utc);
        static Time::TimePoint ToUTC(const Time::TimePoint& local);
        static std::string GetTimeZoneName();
        static int GetTimeZoneOffset();

        // Additional time utilities
        static bool IsDaylightSavingTime(const Time::TimePoint& time);
        static int GetDayOfWeek(
            const Time::TimePoint& time);  // 0 = Sunday, 6 = Saturday
        static int GetDayOfYear(const Time::TimePoint& time);  // 1-366
        static bool IsLeapYear(int year);

        // Time comparison operations
        static bool IsEarlierThan(const TimePoint& t1, const TimePoint& t2);
        static bool IsLaterThan(const TimePoint& t1, const TimePoint& t2);
        static bool IsEqual(const TimePoint& t1, const TimePoint& t2);
        static Duration TimeDifference(const TimePoint& t1,
                                       const TimePoint& t2);

        // Get current time in various formats
        static TimePoint Now();
        static TimePoint UtcNow();
        static std::string GetCurrentTimeString(
            const char* format = FORMAT_DATETIME);

        // Cache control
        static void ClearTimeZoneCache();
        static void UpdateTimeZoneCache();

      private:
        static void UpdateTimers();
        static std::vector<Timer*> timers;
        static Time::TimePoint startTime;
        static Time::TimePoint lastFrameTime;
        static Duration deltaTime;
        static Duration unscaledDeltaTime;
        static double timeScale;
        static double fixedTimeStep;
        static double fixedTimeAccumulator;
        static uint64_t frameCount;
        static double fps;
        static double fpsAccumulator;
        static uint32_t fpsFrameCount;
        static std::map<std::string, TimeZone> timeZoneCache;
    };

    // Helper class for measuring execution time of code blocks
    class ScopedTimer {
      public:
        explicit ScopedTimer(const char* name);
        ~ScopedTimer();

      private:
        const char* name;
        Time::TimePoint startTime;
    };

// Macro for easy scoped timing
#define SCOPED_TIMER(name) Engine::ScopedTimer timer##__LINE__(name)

    // Timer class for handling scheduled callbacks
    class Timer {
      public:
        Timer(const Time::TimerCallback& callback,
              double interval,
              bool repeat = false);
        ~Timer();

        void Start();
        void Stop();
        void Pause();
        void Resume();
        void Reset();

        bool IsActive() const { return isActive; }
        bool IsPaused() const { return isPaused; }
        double GetInterval() const { return interval; }
        double GetTimeRemaining() const;
        uint64_t GetTriggerCount() const { return triggerCount; }

      private:
        friend class Time;
        void Update(double deltaTime);

        Time::TimerCallback callback;
        double interval;
        double currentTime;
        bool repeat;
        bool isActive;
        bool isPaused;
        uint64_t triggerCount;
    };

    // TimeZone class for handling time zone conversions
    class TimeZone {
      public:
        static TimeZone Local();
        static TimeZone UTC();
        static TimeZone FromName(const std::string& name);

        Time::TimePoint ToLocalTime(const Time::TimePoint& utc) const;
        Time::TimePoint ToUTC(const Time::TimePoint& local) const;

        std::string GetName() const;
        int GetOffset() const;  // Offset in seconds from UTC
        bool IsDaylightSavingTime(const Time::TimePoint& time) const;

      public:
        TimeZone() : name("UTC"), cachedOffset(0), hasValidCache(false) {}

      private:
        TimeZone(const std::string& name);
        std::string name;
        mutable int cachedOffset;
        mutable bool hasValidCache;
    };

    // TimeRange class for handling time intervals
    class TimeRange {
      public:
        TimeRange(const Time::TimePoint& start, const Time::TimePoint& end);
        TimeRange(const Time::TimePoint& start, const Time::Duration& duration);

        Time::TimePoint GetStart() const { return start; }
        Time::TimePoint GetEnd() const { return end; }
        Time::Duration GetDuration() const { return end - start; }

        bool Contains(const Time::TimePoint& time) const;
        bool Contains(const TimeRange& other) const;
        bool Overlaps(const TimeRange& other) const;
        bool IsEmpty() const;

        TimeRange Intersect(const TimeRange& other) const;
        TimeRange Union(const TimeRange& other) const;
        std::vector<TimeRange> Subtract(const TimeRange& other) const;

        static TimeRange FromDuration(const Time::TimePoint& start,
                                      double seconds);
        static TimeRange Today();
        static TimeRange ThisWeek();
        static TimeRange ThisMonth();
        static TimeRange ThisYear();

      private:
        Time::TimePoint start;
        Time::TimePoint end;
    };

}  // namespace Engine
