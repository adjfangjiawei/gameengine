
#include "ConsoleUtils.h"

#include <iostream>

#include "Platform.h"

#if PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// Windows specific constants
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#endif

namespace Engine {
    namespace Console {

        ConsoleFormatter &ConsoleFormatter::Get() {
            static ConsoleFormatter instance;
            return instance;
        }

        ConsoleFormatter::ConsoleFormatter() { InitializeColorSupport(); }

        void ConsoleFormatter::InitializeColorSupport() {
#if PLATFORM_WINDOWS
            // Check if Windows supports ANSI escape sequences
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD mode = 0;
                if (GetConsoleMode(hOut, &mode)) {
                    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    if (SetConsoleMode(hOut, mode)) {
                        colorSupport = ColorSupport::TrueColor;
                        return;
                    }
                }
                // If virtual terminal processing is not supported, fall back to
                // basic colors
                colorSupport = ColorSupport::Basic;
            } else {
                colorEnabled = false;
            }
#else
            // Check if output is a terminal
            if (!isatty(STDOUT_FILENO)) {
                colorEnabled = false;
                return;
            }

            // Check TERM environment variable
            const char *term = std::getenv("TERM");
            if (!term) {
                colorEnabled = false;
                return;
            }

            // Check for color support level
            std::string termStr(term);
            if (termStr == "dumb") {
                colorEnabled = false;
                return;
            }

            // Check for true color support
            const char *colorterm = std::getenv("COLORTERM");
            if (colorterm) {
                std::string colortermStr(colorterm);
                if (colortermStr == "truecolor" || colortermStr == "24bit") {
                    colorSupport = ColorSupport::TrueColor;
                    return;
                }
            }

            // Check for 256 color support
            if (termStr.find("256color") != std::string::npos) {
                colorSupport = ColorSupport::Extended;
                return;
            }

            // Check for basic color support
            if (termStr.find("color") != std::string::npos ||
                termStr == "xterm" || termStr == "linux" ||
                termStr == "screen") {
                colorSupport = ColorSupport::Basic;
                return;
            }

            // Default to no color support
            colorEnabled = false;
#endif
        }

        ColorSupport ConsoleFormatter::GetColorSupport() const {
            return colorSupport;
        }

        void ConsoleFormatter::DetectColorSupport() {
            InitializeColorSupport();
        }

        std::string ConsoleFormatter::GetAnsiCode(int code) const {
            if (!colorEnabled) return "";
            return "\033[" + std::to_string(code) + "m";
        }

        std::string ConsoleFormatter::GetColorCode(Color color,
                                                   bool background) const {
            if (!colorEnabled) return "";

            int baseCode = background ? 40 : 30;
            int brightCode = background ? 100 : 90;

            switch (color) {
                case Color::Black:
                    return GetAnsiCode(baseCode + 0);
                case Color::Red:
                    return GetAnsiCode(baseCode + 1);
                case Color::Green:
                    return GetAnsiCode(baseCode + 2);
                case Color::Yellow:
                    return GetAnsiCode(baseCode + 3);
                case Color::Blue:
                    return GetAnsiCode(baseCode + 4);
                case Color::Magenta:
                    return GetAnsiCode(baseCode + 5);
                case Color::Cyan:
                    return GetAnsiCode(baseCode + 6);
                case Color::White:
                    return GetAnsiCode(baseCode + 7);
                case Color::BrightBlack:
                    return GetAnsiCode(brightCode + 0);
                case Color::BrightRed:
                    return GetAnsiCode(brightCode + 1);
                case Color::BrightGreen:
                    return GetAnsiCode(brightCode + 2);
                case Color::BrightYellow:
                    return GetAnsiCode(brightCode + 3);
                case Color::BrightBlue:
                    return GetAnsiCode(brightCode + 4);
                case Color::BrightMagenta:
                    return GetAnsiCode(brightCode + 5);
                case Color::BrightCyan:
                    return GetAnsiCode(brightCode + 6);
                case Color::BrightWhite:
                    return GetAnsiCode(brightCode + 7);
                default:
                    return GetAnsiCode(0);
            }
        }

        std::string ConsoleFormatter::SetForeground(Color color) const {
            return GetColorCode(color, false);
        }

        std::string ConsoleFormatter::SetBackground(Color color) const {
            return GetColorCode(color, true);
        }

        std::string ConsoleFormatter::SetForegroundRGB(uint8 r,
                                                       uint8 g,
                                                       uint8 b) const {
            if (!colorEnabled || colorSupport != ColorSupport::TrueColor)
                return "";
            return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) +
                   ";" + std::to_string(b) + "m";
        }

        std::string ConsoleFormatter::SetBackgroundRGB(uint8 r,
                                                       uint8 g,
                                                       uint8 b) const {
            if (!colorEnabled || colorSupport != ColorSupport::TrueColor)
                return "";
            return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) +
                   ";" + std::to_string(b) + "m";
        }

        std::string ConsoleFormatter::SetStyle(Style style) const {
            if (!colorEnabled) return "";
            return GetAnsiCode(static_cast<int>(style));
        }

        std::string ConsoleFormatter::Reset() const { return GetAnsiCode(0); }

        std::string ConsoleFormatter::Colorize(const std::string &text,
                                               Color color) const {
            if (!colorEnabled) return text;
            return SetForeground(color) + text + Reset();
        }

        std::string ConsoleFormatter::Colorize(const std::string &text,
                                               Color color,
                                               Style style) const {
            if (!colorEnabled) return text;
            return SetForeground(color) + SetStyle(style) + text + Reset();
        }

        std::string ConsoleFormatter::ColorizeRGB(const std::string &text,
                                                  uint8 r,
                                                  uint8 g,
                                                  uint8 b) const {
            if (!colorEnabled || colorSupport != ColorSupport::TrueColor)
                return text;
            return SetForegroundRGB(r, g, b) + text + Reset();
        }

        std::string ConsoleFormatter::MoveCursorUp(int lines) const {
            if (!colorEnabled) return "";
            return "\033[" + std::to_string(lines) + "A";
        }

        std::string ConsoleFormatter::MoveCursorDown(int lines) const {
            if (!colorEnabled) return "";
            return "\033[" + std::to_string(lines) + "B";
        }

        std::string ConsoleFormatter::MoveCursorRight(int columns) const {
            if (!colorEnabled) return "";
            return "\033[" + std::to_string(columns) + "C";
        }

        std::string ConsoleFormatter::MoveCursorLeft(int columns) const {
            if (!colorEnabled) return "";
            return "\033[" + std::to_string(columns) + "D";
        }

        std::string ConsoleFormatter::MoveCursorTo(int line, int column) const {
            if (!colorEnabled) return "";
            return "\033[" + std::to_string(line) + ";" +
                   std::to_string(column) + "H";
        }

        std::string ConsoleFormatter::ClearLine() const {
            if (!colorEnabled) return "";
            return "\033[2K";
        }

        std::string ConsoleFormatter::ClearScreenToEnd() const {
            if (!colorEnabled) return "";
            return "\033[J";
        }

        std::string ConsoleFormatter::ClearScreenToBeginning() const {
            if (!colorEnabled) return "";
            return "\033[1J";
        }

        std::string ConsoleFormatter::ClearScreen() const {
            if (!colorEnabled) return "";
            return "\033[2J";
        }

    }  // namespace Console
}  // namespace Engine
