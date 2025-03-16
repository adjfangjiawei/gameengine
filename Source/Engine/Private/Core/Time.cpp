
#include "Core/Time.h"

#include <math.h>

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "Core/Log/LogSystem.h"
#include "Core/String/StringUtils.h"

namespace Engine {

    // Static member initialization
    std::vector<Timer*> Time::timers;
    Time::TimePoint Time::startTime;
    Time::TimePoint Time::lastFrameTime;
    Time::Duration Time::deltaTime{0.0};
    Time::Duration Time::unscaledDeltaTime{0.0};
    double Time::timeScale = 1.0;
    double Time::fixedTimeStep = 1.0 / 60.0;
    double Time::fixedTimeAccumulator = 0.0;
    uint64_t Time::frameCount = 0;
    double Time::fps = 0.0;
    double Time::fpsAccumulator = 0.0;
    uint32_t Time::fpsFrameCount = 0;
    std::map<std::string, TimeZone> Time::timeZoneCache;

    void Time::Initialize() {
        startTime = Clock::now();
        lastFrameTime = startTime;
        deltaTime = Duration{0.0};
        unscaledDeltaTime = Duration{0.0};
        timeScale = 1.0;
        fixedTimeStep = 1.0 / 60.0;
        fixedTimeAccumulator = 0.0;
        frameCount = 0;
        fps = 0.0;
        fpsAccumulator = 0.0;
        fpsFrameCount = 0;
        UpdateTimeZoneCache();
    }

    void Time::Update() {
        TimePoint currentTime = Clock::now();
        unscaledDeltaTime = currentTime - lastFrameTime;
        deltaTime = unscaledDeltaTime * timeScale;
        lastFrameTime = currentTime;
        frameCount++;

        // Update FPS calculation
        fpsAccumulator += unscaledDeltaTime.count();
        fpsFrameCount++;
        if (fpsAccumulator >= 1.0) {
            fps = static_cast<double>(fpsFrameCount) / fpsAccumulator;
            fpsAccumulator = 0.0;
            fpsFrameCount = 0;
        }

        // Update fixed time accumulator
        fixedTimeAccumulator += deltaTime.count();

        // Update timers
        UpdateTimers();
    }

    double Time::GetTime() {
        return std::chrono::duration<double>(Clock::now() - startTime).count();
    }

    double Time::GetDeltaTime() { return deltaTime.count(); }

    double Time::GetUnscaledDeltaTime() { return unscaledDeltaTime.count(); }

    double Time::GetTimeScale() { return timeScale; }

    void Time::SetTimeScale(double scale) { timeScale = std::max(0.0, scale); }

    double Time::GetFixedTimeStep() { return fixedTimeStep; }

    void Time::SetFixedTimeStep(double step) {
        fixedTimeStep = std::max(0.001, step);
    }

    uint64_t Time::GetFrameCount() { return frameCount; }

    double Time::GetFPS() { return fps; }

    double Time::GetFixedTime() { return fixedTimeAccumulator; }

    bool Time::IsTimeForFixedUpdate() {
        if (fixedTimeAccumulator >= fixedTimeStep) {
            fixedTimeAccumulator -= fixedTimeStep;
            return true;
        }
        return false;
    }

    std::string Time::GetSystemTimeString() {
        return GetCurrentTimeString(FORMAT_DATETIME);
    }

    std::string Time::FormatTime(const TimePoint& time, const char* format) {
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time));
        std::stringstream ss;
        ss << std::put_time(std::localtime(&timeT), format);
        return ss.str();
    }

    std::string Time::FormatTime(const TimePoint& time,
                                 const std::string& format) {
        return FormatTime(time, format.c_str());
    }

    bool Time::ParseTime(const std::string& timeStr,
                         const char* format,
                         TimePoint& outTime) {
        std::tm tm = {};
        std::stringstream ss(timeStr);
        ss >> std::get_time(&tm, format);

        if (ss.fail()) {
            LOG_WARNING("Failed to parse time string: {}", timeStr);
            return false;
        }

        auto timeT = std::mktime(&tm);
        outTime = std::chrono::time_point_cast<Clock::duration>(
            std::chrono::system_clock::from_time_t(timeT));
        return true;
    }

    bool Time::ParseTime(const std::string& timeStr,
                         const std::string& format,
                         TimePoint& outTime) {
        return ParseTime(timeStr, format.c_str(), outTime);
    }

    Timer* Time::CreateTimer(const TimerCallback& callback,
                             double interval,
                             bool repeat) {
        Timer* timer = new Timer(callback, interval, repeat);
        timer->Start();
        timers.push_back(timer);
        return timer;
    }

    void Time::DestroyTimer(Timer* timer) {
        if (!timer) return;

        auto it = std::find(timers.begin(), timers.end(), timer);
        if (it != timers.end()) {
            timers.erase(it);
            delete timer;
        }
    }

    void Time::PauseTimer(Timer* timer) {
        if (timer) {
            timer->Pause();
        }
    }

    void Time::ResumeTimer(Timer* timer) {
        if (timer) {
            timer->Resume();
        }
    }

    void Time::UpdateTimers() {
        // Make a copy of the timers vector to safely handle removals during
        // iteration
        std::vector<Timer*> currentTimers = timers;

        for (auto* timer : currentTimers) {
            if (timer->IsActive() && !timer->IsPaused()) {
                timer->Update(deltaTime.count());
            }
        }
    }

    Time::TimePoint Time::AddDays(const Time::TimePoint& time, int days) {
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time));
        std::tm* tm = std::localtime(&timeT);
        tm->tm_mday += days;
        auto newTimeT = std::mktime(tm);

        return std::chrono::time_point_cast<Clock::duration>(
            std::chrono::system_clock::from_time_t(newTimeT));
    }

    Time::TimePoint Time::AddMonths(const Time::TimePoint& time, int months) {
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time));
        std::tm* tm = std::localtime(&timeT);
        tm->tm_mon += months;
        auto newTimeT = std::mktime(tm);

        return std::chrono::time_point_cast<Clock::duration>(
            std::chrono::system_clock::from_time_t(newTimeT));
    }

    Time::TimePoint Time::AddYears(const Time::TimePoint& time, int years) {
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time));
        std::tm* tm = std::localtime(&timeT);
        tm->tm_year += years;
        auto newTimeT = std::mktime(tm);

        return std::chrono::time_point_cast<Clock::duration>(
            std::chrono::system_clock::from_time_t(newTimeT));
    }

    int Time::DaysBetween(const Time::TimePoint& t1,
                          const Time::TimePoint& t2) {
        // Convert to system_clock time points for date calculations
        auto sys_t1 =
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                std::chrono::system_clock::now() + (t1 - Clock::now()));
        auto sys_t2 =
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                std::chrono::system_clock::now() + (t2 - Clock::now()));

        // Get days since epoch for both time points
        auto days_t1 = std::chrono::duration_cast<std::chrono::hours>(
                           sys_t1.time_since_epoch())
                           .count() /
                       24;
        auto days_t2 = std::chrono::duration_cast<std::chrono::hours>(
                           sys_t2.time_since_epoch())
                           .count() /
                       24;

        return static_cast<int>(std::abs(days_t2 - days_t1));
    }

    Time::TimePoint Time::ToLocalTime(const Time::TimePoint& utc) {
        return TimeZone::Local().ToLocalTime(utc);
    }

    Time::TimePoint Time::ToUTC(const Time::TimePoint& local) {
        return TimeZone::Local().ToUTC(local);
    }

    std::string Time::GetTimeZoneName() { return TimeZone::Local().GetName(); }

    int Time::GetTimeZoneOffset() { return TimeZone::Local().GetOffset(); }

    bool Time::IsDaylightSavingTime(const Time::TimePoint& time) {
        return TimeZone::Local().IsDaylightSavingTime(time);
    }

    int Time::GetDayOfWeek(const Time::TimePoint& time) {
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time));
        std::tm* tm = std::localtime(&timeT);
        return tm->tm_wday;  // 0 = Sunday, 6 = Saturday
    }

    int Time::GetDayOfYear(const Time::TimePoint& time) {
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time));
        std::tm* tm = std::localtime(&timeT);
        return tm->tm_yday + 1;  // tm_yday is 0-365, but we want 1-366
    }

    bool Time::IsLeapYear(int year) {
        return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    }

    // Time comparison operations
    bool Time::IsEarlierThan(const TimePoint& t1, const TimePoint& t2) {
        return t1 < t2;
    }

    bool Time::IsLaterThan(const TimePoint& t1, const TimePoint& t2) {
        return t1 > t2;
    }

    bool Time::IsEqual(const TimePoint& t1, const TimePoint& t2) {
        return t1 == t2;
    }

    Time::Duration Time::TimeDifference(const TimePoint& t1,
                                        const TimePoint& t2) {
        return t2 - t1;
    }

    Time::TimePoint Time::Now() { return Clock::now(); }

    Time::TimePoint Time::UtcNow() { return ToUTC(Now()); }

    std::string Time::GetCurrentTimeString(const char* format) {
        return FormatTime(Now(), format);
    }

    void Time::ClearTimeZoneCache() { timeZoneCache.clear(); }

    void Time::UpdateTimeZoneCache() {
        // Cache local timezone
        timeZoneCache["Local"] = TimeZone::Local();
        timeZoneCache["UTC"] = TimeZone::UTC();
    }

    // ScopedTimer implementation
    ScopedTimer::ScopedTimer(const char* name) : name(name) {
        startTime = Time::Now();
    }

    ScopedTimer::~ScopedTimer() {
        Time::Duration duration = Time::Now() - startTime;
        LOG_INFO("{} took {:.6f} seconds", name, duration.count());
    }

    // TimeRange implementation
    TimeRange::TimeRange(const Time::TimePoint& startTime,
                         const Time::TimePoint& endTime)
        : start(startTime), end(endTime) {
        if (end < start) {
            std::swap(this->start, this->end);
        }
    }

    TimeRange::TimeRange(const Time::TimePoint& startTime,
                         const Time::Duration& duration)
        : start(std::chrono::time_point_cast<Time::Clock::duration>(startTime)),
          end(std::chrono::time_point_cast<Time::Clock::duration>(startTime +
                                                                  duration)) {}

    bool TimeRange::Contains(const Time::TimePoint& time) const {
        return time >= start && time <= end;
    }

    bool TimeRange::Contains(const TimeRange& other) const {
        return other.start >= start && other.end <= end;
    }

    bool TimeRange::Overlaps(const TimeRange& other) const {
        return !(end < other.start || start > other.end);
    }

    bool TimeRange::IsEmpty() const { return end <= start; }

    TimeRange TimeRange::Intersect(const TimeRange& other) const {
        Time::TimePoint intersectStart = std::max(start, other.start);
        Time::TimePoint intersectEnd = std::min(end, other.end);
        return TimeRange(intersectStart, intersectEnd);
    }

    TimeRange TimeRange::Union(const TimeRange& other) const {
        Time::TimePoint unionStart = std::min(start, other.start);
        Time::TimePoint unionEnd = std::max(end, other.end);
        return TimeRange(unionStart, unionEnd);
    }

    std::vector<TimeRange> TimeRange::Subtract(const TimeRange& other) const {
        std::vector<TimeRange> result;

        // No overlap
        if (!Overlaps(other)) {
            result.push_back(*this);
            return result;
        }

        // Left part
        if (start < other.start) {
            result.push_back(TimeRange(start, other.start));
        }

        // Right part
        if (end > other.end) {
            result.push_back(TimeRange(other.end, end));
        }

        return result;
    }

    TimeRange TimeRange::FromDuration(const Time::TimePoint& start,
                                      double seconds) {
        return TimeRange(start, Time::Duration(seconds));
    }

    TimeRange TimeRange::Today() {
        auto now = Time::Now();
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                now));
        std::tm* tm = std::localtime(&timeT);

        // Set to start of day
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        auto startTimeT = std::mktime(tm);

        // Set to end of day
        tm->tm_hour = 23;
        tm->tm_min = 59;
        tm->tm_sec = 59;
        auto endTimeT = std::mktime(tm);

        auto start = std::chrono::time_point_cast<Time::Clock::duration>(
            std::chrono::system_clock::from_time_t(startTimeT));
        auto end = std::chrono::time_point_cast<Time::Clock::duration>(
            std::chrono::system_clock::from_time_t(endTimeT));

        return TimeRange(start, end);
    }

    TimeRange TimeRange::ThisWeek() {
        auto today = Today();
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                today.GetStart()));
        std::tm* tm = std::localtime(&timeT);

        // Adjust to start of week (Sunday)
        int daysToSubtract = tm->tm_wday;
        auto start = today.GetStart() - std::chrono::hours(24 * daysToSubtract);
        auto end = start + std::chrono::hours(24 * 7) - std::chrono::seconds(1);

        return TimeRange(start, end);
    }

    TimeRange TimeRange::ThisMonth() {
        auto now = Time::Now();
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                now));
        std::tm* tm = std::localtime(&timeT);

        // Set to start of month
        tm->tm_mday = 1;
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        auto startTimeT = std::mktime(tm);

        // Set to end of month
        tm->tm_mon++;
        tm->tm_mday = 0;
        tm->tm_hour = 23;
        tm->tm_min = 59;
        tm->tm_sec = 59;
        auto endTimeT = std::mktime(tm);

        auto start = std::chrono::time_point_cast<Time::Clock::duration>(
            std::chrono::system_clock::from_time_t(startTimeT));
        auto end = std::chrono::time_point_cast<Time::Clock::duration>(
            std::chrono::system_clock::from_time_t(endTimeT));

        return TimeRange(start, end);
    }

    TimeRange TimeRange::ThisYear() {
        auto now = Time::Now();
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                now));
        std::tm* tm = std::localtime(&timeT);

        // Set to start of year
        tm->tm_mon = 0;
        tm->tm_mday = 1;
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        auto startTimeT = std::mktime(tm);

        // Set to end of year
        tm->tm_year++;
        tm->tm_mon = 0;
        tm->tm_mday = 0;
        tm->tm_hour = 23;
        tm->tm_min = 59;
        tm->tm_sec = 59;
        auto endTimeT = std::mktime(tm);

        auto start = std::chrono::time_point_cast<Time::Clock::duration>(
            std::chrono::system_clock::from_time_t(startTimeT));
        auto end = std::chrono::time_point_cast<Time::Clock::duration>(
            std::chrono::system_clock::from_time_t(endTimeT));

        return TimeRange(start, end);
    }

    // Include the rest of the existing Time.cpp implementation here...
    // (Keep all the existing methods from the previous implementation)

    // Timer implementation
    Timer::Timer(const Time::TimerCallback& callback,
                 double interval,
                 bool repeat)
        : callback(callback),
          interval(interval),
          currentTime(0.0),
          repeat(repeat),
          isActive(false),
          isPaused(false),
          triggerCount(0) {}

    Timer::~Timer() { Stop(); }

    void Timer::Start() {
        isActive = true;
        isPaused = false;
        currentTime = 0.0;
    }

    void Timer::Stop() { isActive = false; }

    void Timer::Pause() { isPaused = true; }

    void Timer::Resume() { isPaused = false; }

    void Timer::Reset() { currentTime = 0.0; }

    double Timer::GetTimeRemaining() const {
        return std::max(0.0, interval - currentTime);
    }

    void Timer::Update(double deltaTime) {
        if (!isActive || isPaused) return;

        currentTime += deltaTime;

        if (currentTime >= interval) {
            // Timer triggered
            if (callback) {
                callback();
            }

            triggerCount++;

            if (repeat) {
                // For repeating timers, subtract the interval but keep any
                // overflow time
                currentTime = std::fmod(currentTime, interval);
            } else {
                // For non-repeating timers, deactivate after triggering
                isActive = false;
                currentTime = 0.0;
            }
        }
    }

    // TimeZone implementation
    TimeZone TimeZone::Local() { return TimeZone("Local"); }

    TimeZone TimeZone::UTC() { return TimeZone("UTC"); }

    TimeZone TimeZone::FromName(const std::string& name) {
        // In a real implementation, this would validate the timezone name
        // For now, we just return a TimeZone with the given name
        return TimeZone(name);
    }

    TimeZone::TimeZone(const std::string& name)
        : name(name), cachedOffset(0), hasValidCache(false) {}

    Time::TimePoint TimeZone::ToLocalTime(const Time::TimePoint& utc) const {
        if (name == "UTC") {
            // If this is the UTC timezone, then local time is the same as UTC
            // time
            return utc;
        }

        // Convert through system_clock for timezone conversion
        auto sysTime =
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                std::chrono::system_clock::now() + (utc - Time::Clock::now()));
        auto timeT = std::chrono::system_clock::to_time_t(sysTime);

        // Convert to local time
        std::tm* tmLocal = std::localtime(&timeT);
        auto localTimeT = std::mktime(tmLocal);

        auto localSysTime = std::chrono::system_clock::from_time_t(localTimeT);

        // Convert back to high_resolution_clock time point
        return Time::Clock::now() +
               (localSysTime - std::chrono::system_clock::now());
    }

    Time::TimePoint TimeZone::ToUTC(const Time::TimePoint& local) const {
        if (name == "UTC") {
            // If this is the UTC timezone, then UTC time is the same as local
            // time
            return local;
        }

        // Convert through system_clock for timezone conversion
        auto sysTime =
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                std::chrono::system_clock::now() +
                (local - Time::Clock::now()));
        auto timeT = std::chrono::system_clock::to_time_t(sysTime);

        // Convert to UTC time
        std::tm* tmUTC = std::gmtime(&timeT);
        auto utcTimeT = std::mktime(tmUTC);

        auto utcSysTime = std::chrono::system_clock::from_time_t(utcTimeT);

        // Convert back to high_resolution_clock time point
        return Time::Clock::now() +
               (utcSysTime - std::chrono::system_clock::now());
    }

    std::string TimeZone::GetName() const { return name; }

    int TimeZone::GetOffset() const {
        if (!hasValidCache) {
            // Calculate offset between local time and UTC
            auto now = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(now);

            std::tm* tmLocal = std::localtime(&timeT);
            std::tm* tmUTC = std::gmtime(&timeT);

            auto localTimeT = std::mktime(tmLocal);
            auto utcTimeT = std::mktime(tmUTC);

            // Cache the offset in seconds
            cachedOffset =
                static_cast<int>(std::difftime(localTimeT, utcTimeT));
            hasValidCache = true;
        }

        return cachedOffset;
    }

    bool TimeZone::IsDaylightSavingTime(const Time::TimePoint& time) const {
        auto timeT = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time));
        std::tm* tm = std::localtime(&timeT);
        return tm->tm_isdst > 0;
    }
}  // namespace Engine
