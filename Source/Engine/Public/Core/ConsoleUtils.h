
#pragma once

#include <iostream>
#include <string>

#include "CoreTypes.h"

namespace Engine {
    namespace Console {

        // Console colors
        enum class Color {
            Default,
            Black,
            Red,
            Green,
            Yellow,
            Blue,
            Magenta,
            Cyan,
            White,
            BrightBlack,
            BrightRed,
            BrightGreen,
            BrightYellow,
            BrightBlue,
            BrightMagenta,
            BrightCyan,
            BrightWhite
        };

        // Console text styles
        enum class Style {
            Normal = 0,
            Bold = 1,
            Dim = 2,
            Italic = 3,
            Underline = 4,
            Blink = 5,
            Reverse = 7,
            Hidden = 8,
            Strike = 9
        };

        // Console color support level
        enum class ColorSupport {
            None,      // No color support
            Basic,     // Basic 16 colors
            Extended,  // 256 colors
            TrueColor  // 24-bit true color
        };

        class ConsoleFormatter {
          public:
            // Get singleton instance
            static ConsoleFormatter &Get();

            // Color support
            ColorSupport GetColorSupport() const;
            void DetectColorSupport();
            bool IsColorEnabled() const { return colorEnabled; }
            void EnableColor(bool enable) { colorEnabled = enable; }

            // Foreground colors
            std::string SetForeground(Color color) const;
            std::string SetForegroundRGB(uint8 r, uint8 g, uint8 b) const;

            // Background colors
            std::string SetBackground(Color color) const;
            std::string SetBackgroundRGB(uint8 r, uint8 g, uint8 b) const;

            // Text styles
            std::string SetStyle(Style style) const;

            // Reset all attributes
            std::string Reset() const;

            // Cursor movement
            std::string MoveCursorUp(int lines) const;
            std::string MoveCursorDown(int lines) const;
            std::string MoveCursorRight(int columns) const;
            std::string MoveCursorLeft(int columns) const;
            std::string MoveCursorTo(int line, int column) const;

            // Line operations
            std::string ClearLine() const;
            std::string ClearScreenToEnd() const;
            std::string ClearScreenToBeginning() const;
            std::string ClearScreen() const;

            // Convenience methods for colored text
            std::string Colorize(const std::string &text, Color color) const;
            std::string Colorize(const std::string &text,
                                 Color color,
                                 Style style) const;
            std::string ColorizeRGB(const std::string &text,
                                    uint8 r,
                                    uint8 g,
                                    uint8 b) const;

          private:
            ConsoleFormatter();
            ConsoleFormatter(const ConsoleFormatter &) = delete;
            ConsoleFormatter &operator=(const ConsoleFormatter &) = delete;

            bool colorEnabled{true};
            ColorSupport colorSupport{ColorSupport::None};

            void InitializeColorSupport();
            std::string GetAnsiCode(int code) const;
            std::string GetColorCode(Color color,
                                     bool background = false) const;
        };

        // Global convenience functions
        inline std::string ColoredText(const std::string &text, Color color) {
            return ConsoleFormatter::Get().Colorize(text, color);
        }

        inline std::string StyledText(const std::string &text,
                                      Color color,
                                      Style style) {
            return ConsoleFormatter::Get().Colorize(text, color, style);
        }

        inline std::string RGBText(const std::string &text,
                                   uint8 r,
                                   uint8 g,
                                   uint8 b) {
            return ConsoleFormatter::Get().ColorizeRGB(text, r, g, b);
        }

        // RAII style color scope
        class ScopedColor {
          public:
            ScopedColor(Color color) {
                std::cout << ConsoleFormatter::Get().SetForeground(color);
            }

            ~ScopedColor() { std::cout << ConsoleFormatter::Get().Reset(); }
        };

        // RAII style style scope
        class ScopedStyle {
          public:
            ScopedStyle(Style style) {
                std::cout << ConsoleFormatter::Get().SetStyle(style);
            }

            ~ScopedStyle() { std::cout << ConsoleFormatter::Get().Reset(); }
        };

    }  // namespace Console
}  // namespace Engine

// Convenience macros
#define CONSOLE_COLOR(color) Engine::Console::ScopedColor _scopedColor(color)
#define CONSOLE_STYLE(style) Engine::Console::ScopedStyle _scopedStyle(style)
