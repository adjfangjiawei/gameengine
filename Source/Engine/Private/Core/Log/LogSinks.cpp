
#include "Core/Log/LogSinks.h"

#include <iostream>

#include "Core/FileSystem/FileSystem.h"
#include "Core/Log/LogSystem.h"
#include "Core/String/StringUtils.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <syslog.h>
#endif

namespace Engine {

    // ConsoleLogSink implementation
    ConsoleLogSink::ConsoleLogSink(bool useColors) : m_useColors(useColors) {}

    void ConsoleLogSink::Write(const LogMessage &message) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_useColors) {
            SetConsoleColor(message.level);
        }

        std::string formattedMessage =
            LogSystem::Get().GetFormat().messageFormat;
        formattedMessage = LogSystem::Get().FormatMessage(message);

        if (message.level >= LogLevel::Error) {
            std::cerr << formattedMessage << std::endl;
        } else {
            std::cout << formattedMessage << std::endl;
        }

        if (m_useColors) {
            ResetConsoleColor();
        }
    }

    void ConsoleLogSink::Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout.flush();
        std::cerr.flush();
    }

    void ConsoleLogSink::SetConsoleColor(LogLevel level) {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color;

        switch (level) {
            case LogLevel::Trace:
                color = FOREGROUND_INTENSITY;
                break;
            case LogLevel::Debug:
                color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Info:
                color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Warning:
                color =
                    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Error:
                color = FOREGROUND_RED | FOREGROUND_INTENSITY;
                break;
            case LogLevel::Fatal:
                color = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                break;
            default:
                color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                break;
        }

        SetConsoleTextAttribute(hConsole, color);
#else
        const char *color;

        switch (level) {
            case LogLevel::Trace:
                color = "\033[37m";
                break;  // White
            case LogLevel::Debug:
                color = "\033[36m";
                break;  // Cyan
            case LogLevel::Info:
                color = "\033[32m";
                break;  // Green
            case LogLevel::Warning:
                color = "\033[33m";
                break;  // Yellow
            case LogLevel::Error:
                color = "\033[31m";
                break;  // Red
            case LogLevel::Fatal:
                color = "\033[35m";
                break;  // Magenta
            default:
                color = "\033[0m";
                break;  // Reset
        }

        std::cout << color;
#endif
    }

    void ConsoleLogSink::ResetConsoleColor() {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(
            hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
        std::cout << "\033[0m";
#endif
    }

    // FileLogSink implementation
    FileLogSink::FileLogSink(const std::string &filename,
                             bool append,
                             size_t maxFileSize,
                             size_t maxFiles)
        : m_filename(filename),
          m_maxFileSize(maxFileSize),
          m_maxFiles(maxFiles),
          m_currentSize(0) {
        // Ensure the log directory exists
        auto &fs = FileSystem::Get();
        std::string dirPath = fs.GetDirectoryName(m_filename);
        if (!dirPath.empty() && !fs.CreateDirectory(dirPath)) {
            std::cerr << "Failed to create log directory: " << dirPath
                      << std::endl;
        }

        OpenLogFile(append);
    }

    FileLogSink::~FileLogSink() {
        try {
            if (m_fileHandle) {
                m_fileHandle->Flush();
                m_fileHandle->Close();
                m_fileHandle.reset();
            }
            if (m_file.is_open()) {
                m_file.flush();
                m_file.close();
            }
        } catch (const std::exception &e) {
            std::cerr << "Error during FileLogSink destruction: " << e.what()
                      << std::endl;
        }
    }

    void FileLogSink::Write(const LogMessage &message) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Try to reopen file if it's not open
        if (!m_fileHandle || !m_file.is_open()) {
            OpenLogFile(true);
            if (!m_fileHandle || !m_file.is_open()) {
                std::cerr << "Failed to open log file for writing: "
                          << m_filename << std::endl;
                return;
            }
        }

        try {
            std::string formattedMessage =
                LogSystem::Get().FormatMessage(message);
            formattedMessage += "\n";

            size_t written = m_fileHandle->Write(formattedMessage.c_str(),
                                                 formattedMessage.length());
            if (written != formattedMessage.length()) {
                throw std::runtime_error("Failed to write to log file");
            }

            // Also write to ofstream for formatted output
            m_file << formattedMessage;

            m_currentSize += formattedMessage.length();

            if (m_currentSize >= m_maxFileSize) {
                RollFiles();
            }
        } catch (const std::exception &e) {
            std::cerr << "Error writing to log file: " << e.what() << std::endl;
            // Try to recover by closing and reopening the file
            m_file.close();
            if (m_fileHandle) {
                m_fileHandle->Close();
                m_fileHandle.reset();
            }
            OpenLogFile(true);
        }
    }

    void FileLogSink::Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            if (m_fileHandle) {
                m_fileHandle->Flush();
            }
            if (m_file.is_open()) {
                m_file.flush();
            }
        } catch (const std::exception &e) {
            std::cerr << "Error flushing log file: " << e.what() << std::endl;
        }
    }

    void FileLogSink::RollFiles() {
        try {
            m_file.close();

            auto &fs = FileSystem::Get();
            // Create a backup directory if it doesn't exist
            std::string backupDir =
                fs.CombinePaths(fs.GetDirectoryName(m_filename), "backup");
            fs.CreateDirectory(backupDir);

            // Delete oldest file if it exists
            std::string oldestFile = GetBackupFileName(m_maxFiles - 1);
            if (fs.Exists(oldestFile)) {
                fs.DeleteFile(oldestFile);
            }

            // Rename existing backup files
            for (size_t i = m_maxFiles - 2; i > 0; --i) {
                std::string oldName = GetBackupFileName(i);
                std::string newName = GetBackupFileName(i + 1);
                if (fs.Exists(oldName)) {
                    fs.MoveFile(oldName, newName);
                }
            }

            // Rename current file to .1
            if (fs.Exists(m_filename)) {
                fs.MoveFile(m_filename, GetBackupFileName(1));
            }

            // Open new file
            OpenLogFile(false);
        } catch (const std::exception &e) {
            std::cerr << "Error during log file rotation: " << e.what()
                      << std::endl;
            // Try to recover by opening a new file
            OpenLogFile(false);
        }
    }

    std::string FileLogSink::GetBackupFileName(size_t index) const {
        auto &fs = FileSystem::Get();
        std::string backupDir =
            fs.CombinePaths(fs.GetDirectoryName(m_filename), "backup");
        std::string filename =
            fs.GetFileName(m_filename) + "." + std::to_string(index);
        return fs.CombinePaths(backupDir, filename);
    }

    void FileLogSink::OpenLogFile(bool append) {
        try {
            FileMode mode = append ? FileMode::Append : FileMode::Write;
            auto file = FileSystem::Get().OpenFile(m_filename, mode);
            if (!file) {
                throw std::runtime_error("Failed to open log file");
            }

            m_currentSize = append && FileSystem::Get().Exists(m_filename)
                                ? file->GetSize()
                                : 0;

            m_file = std::ofstream(
                m_filename,
                std::ios::out | (append ? std::ios::app : std::ios::trunc));

            if (!m_file.is_open()) {
                throw std::runtime_error("Failed to open log file stream");
            }
        } catch (const std::exception &e) {
            std::cerr << "Error opening log file: " << e.what() << std::endl;
            // Try to open in a fallback location
            try {
                std::string fallbackPath = "fallback.log";
                auto file =
                    FileSystem::Get().OpenFile(fallbackPath, FileMode::Append);
                if (file) {
                    m_filename = fallbackPath;
                    m_currentSize = file->GetSize();
                    m_file = std::ofstream(fallbackPath,
                                           std::ios::out | std::ios::app);
                }
            } catch (...) {
                // If fallback fails, we'll leave the file closed
            }
        }
    }

    // DebugLogSink implementation
    void DebugLogSink::Write(const LogMessage &message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string formattedMessage = LogSystem::Get().FormatMessage(message);

#ifdef _WIN32
        OutputDebugStringA((formattedMessage + "\n").c_str());
#else
        int priority;
        switch (message.level) {
            case LogLevel::Trace:
                priority = LOG_DEBUG;
                break;
            case LogLevel::Debug:
                priority = LOG_DEBUG;
                break;
            case LogLevel::Info:
                priority = LOG_INFO;
                break;
            case LogLevel::Warning:
                priority = LOG_WARNING;
                break;
            case LogLevel::Error:
                priority = LOG_ERR;
                break;
            case LogLevel::Fatal:
                priority = LOG_CRIT;
                break;
            default:
                priority = LOG_INFO;
                break;
        }

        // Open syslog connection if not already opened
        static bool syslogOpened = false;
        if (!syslogOpened) {
            openlog("GameEngine", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
            syslogOpened = true;
        }

        syslog(priority, "%s", formattedMessage.c_str());
#endif
    }

    void DebugLogSink::Flush() {
        // No need to flush for debug output
#ifndef _WIN32
        // Ensure all messages are written to syslog
        closelog();
#endif
    }

    // RotatingFileLogSink implementation
    RotatingFileLogSink::RotatingFileLogSink(const std::string &filenamePattern,
                                             RotationInterval interval)
        : m_filenamePattern(filenamePattern), m_interval(interval) {
        // Ensure the directory exists
        auto &fs = FileSystem::Get();
        std::string dirPath = fs.GetDirectoryName(filenamePattern);
        if (!dirPath.empty()) {
            fs.CreateDirectory(dirPath);
        }

        CheckRotation();
    }

    RotatingFileLogSink::~RotatingFileLogSink() {
        if (m_currentFile) {
            m_currentFile->Close();
        }
    }

    void RotatingFileLogSink::Write(const LogMessage &message) {
        std::lock_guard<std::mutex> lock(m_mutex);

        CheckRotation();

        if (!m_currentFile) {
            std::string filename = GetCurrentFileName();
            m_currentFile =
                FileSystem::Get().OpenFile(filename, FileMode::Append);
            if (!m_currentFile) {
                // If we can't open the file, try to write to a fallback
                // location
                m_currentFile = FileSystem::Get().OpenFile(
                    "Saved/Logs/fallback.log", FileMode::Append);
                if (!m_currentFile) {
                    return;  // If still can't open, give up
                }
            }
        }

        std::string formattedMessage =
            LogSystem::Get().FormatMessage(message) + "\n";
        m_currentFile->Write(formattedMessage.c_str(),
                             formattedMessage.length());
    }

    void RotatingFileLogSink::Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_currentFile) {
            m_currentFile->Flush();
        }
    }

    void RotatingFileLogSink::CheckRotation() {
        if (ShouldRotate()) {
            if (m_currentFile) {
                m_currentFile->Close();
                m_currentFile.reset();
            }

            std::string filename = GetCurrentFileName();
            m_currentFile =
                FileSystem::Get().OpenFile(filename, FileMode::Append);

            // Calculate next rotation time
            auto now = std::chrono::system_clock::now();
            auto currentTime = std::chrono::system_clock::to_time_t(now);
            std::tm *localTime = std::localtime(&currentTime);

            // Reset time to start of next period
            localTime->tm_hour = 0;
            localTime->tm_min = 0;
            localTime->tm_sec = 0;

            switch (m_interval) {
                case RotationInterval::Daily:
                    localTime->tm_mday += 1;
                    break;
                case RotationInterval::Weekly:
                    localTime->tm_mday += 7 - localTime->tm_wday;
                    break;
                case RotationInterval::Monthly:
                    localTime->tm_mday = 1;
                    localTime->tm_mon += 1;
                    break;
            }

            m_nextRotation =
                std::chrono::system_clock::from_time_t(std::mktime(localTime));
        }
    }

    std::string RotatingFileLogSink::GetCurrentFileName() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), m_filenamePattern.c_str());
        return ss.str();
    }

    bool RotatingFileLogSink::ShouldRotate() const {
        return !m_currentFile ||
               std::chrono::system_clock::now() >= m_nextRotation;
    }

    // MemoryLogSink implementation
    MemoryLogSink::MemoryLogSink(size_t maxMessages)
        : m_maxMessages(maxMessages) {}

    void MemoryLogSink::Write(const LogMessage &message) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Add new message
        m_messages.push(message);

        // Remove oldest messages if we exceed the maximum
        while (m_messages.size() > m_maxMessages) {
            m_messages.pop();
        }
    }

    void MemoryLogSink::Flush() {
        // Memory sink doesn't need flushing
    }

    std::vector<LogMessage> MemoryLogSink::GetMessages() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<LogMessage> messages;
        messages.reserve(m_messages.size());

        auto tempQueue = m_messages;
        while (!tempQueue.empty()) {
            messages.push_back(tempQueue.front());
            tempQueue.pop();
        }

        return messages;
    }

    void MemoryLogSink::Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<LogMessage> empty;
        std::swap(m_messages, empty);
    }

    // AsyncLogSink implementation
    AsyncLogSink::AsyncLogSink(std::unique_ptr<ILogSink> sink)
        : m_sink(std::move(sink)), m_stopping(false) {
        m_processingThread = std::thread(&AsyncLogSink::ProcessMessages, this);
    }

    AsyncLogSink::~AsyncLogSink() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_condition.notify_all();

        if (m_processingThread.joinable()) {
            m_processingThread.join();
        }
    }

    void AsyncLogSink::Write(const LogMessage &message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messageQueue.push(message);
        m_condition.notify_one();
    }

    void AsyncLogSink::Flush() {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool wasEmpty = m_messageQueue.empty();

        if (!wasEmpty) {
            m_condition.wait(lock, [this] { return m_messageQueue.empty(); });
        }

        lock.unlock();
        m_sink->Flush();
    }

    void AsyncLogSink::ProcessMessages() {
        while (true) {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_condition.wait(
                lock, [this] { return !m_messageQueue.empty() || m_stopping; });

            if (m_stopping && m_messageQueue.empty()) {
                break;
            }

            // Process all available messages
            std::queue<LogMessage> messagesToProcess;
            std::swap(messagesToProcess, m_messageQueue);

            lock.unlock();

            // Write all messages
            while (!messagesToProcess.empty()) {
                m_sink->Write(messagesToProcess.front());
                messagesToProcess.pop();
            }

            lock.lock();
            if (m_messageQueue.empty()) {
                m_condition.notify_all();
            }
        }
    }

    // CompositLogSink implementation
    void CompositLogSink::AddSink(std::unique_ptr<ILogSink> sink) {
        if (sink) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_sinks.push_back(std::move(sink));
        }
    }

    void CompositLogSink::RemoveSink(ILogSink *sink) {
        if (!sink) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it =
            std::find_if(m_sinks.begin(), m_sinks.end(), [sink](const auto &s) {
                return s.get() == sink;
            });

        if (it != m_sinks.end()) {
            m_sinks.erase(it);
        }
    }

    void CompositLogSink::Write(const LogMessage &message) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Store any exceptions that occur during writing
        std::vector<std::exception_ptr> exceptions;

        for (const auto &sink : m_sinks) {
            try {
                sink->Write(message);
            } catch (...) {
                exceptions.push_back(std::current_exception());
            }
        }

        // If any writes failed, throw an exception with all error messages
        if (!exceptions.empty()) {
            std::string errorMsg =
                "Errors occurred in CompositLogSink::Write:\n";
            for (const auto &exc : exceptions) {
                try {
                    std::rethrow_exception(exc);
                } catch (const std::exception &e) {
                    errorMsg += std::string("- ") + e.what() + "\n";
                }
            }
            throw std::runtime_error(errorMsg);
        }
    }

    void CompositLogSink::Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<std::exception_ptr> exceptions;

        for (const auto &sink : m_sinks) {
            try {
                sink->Flush();
            } catch (...) {
                exceptions.push_back(std::current_exception());
            }
        }

        if (!exceptions.empty()) {
            std::string errorMsg =
                "Errors occurred in CompositLogSink::Flush:\n";
            for (const auto &exc : exceptions) {
                try {
                    std::rethrow_exception(exc);
                } catch (const std::exception &e) {
                    errorMsg += std::string("- ") + e.what() + "\n";
                }
            }
            throw std::runtime_error(errorMsg);
        }
    }

    // FilteredLogSink implementation
    FilteredLogSink::FilteredLogSink(std::unique_ptr<ILogSink> sink,
                                     FilterFunction filter)
        : m_sink(std::move(sink)), m_filter(std::move(filter)) {
        if (!m_sink) {
            throw std::invalid_argument(
                "Sink cannot be null in FilteredLogSink");
        }
        if (!m_filter) {
            throw std::invalid_argument(
                "Filter function cannot be null in FilteredLogSink");
        }
    }

    void FilteredLogSink::Write(const LogMessage &message) {
        try {
            if (m_filter(message)) {
                m_sink->Write(message);
            }
        } catch (const std::exception &e) {
            throw std::runtime_error(
                std::string("Error in FilteredLogSink::Write: ") + e.what());
        }
    }

    void FilteredLogSink::Flush() {
        try {
            m_sink->Flush();
        } catch (const std::exception &e) {
            throw std::runtime_error(
                std::string("Error in FilteredLogSink::Flush: ") + e.what());
        }
    }

}  // namespace Engine
