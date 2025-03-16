
#pragma once

#include <sstream>
#include <string>
#include <utility>

namespace Engine {

    // Base case: no more arguments to format
    inline std::string FormatString(const std::string &format) {
        return format;
    }

    // Recursive case: format one argument and continue with the rest
    template <typename T, typename... Args>
    std::string FormatString(const std::string &format,
                             T &&value,
                             Args &&...args) {
        std::string result;
        size_t pos = 0;
        size_t lastPos = 0;

        // Find the first {} placeholder
        pos = format.find("{}", lastPos);
        if (pos == std::string::npos) {
            // No placeholder found, return the format string as is
            return format;
        }

        // Add the part before the placeholder
        result = format.substr(0, pos);

        // Convert the current value to string and add it
        std::stringstream ss;
        ss << std::forward<T>(value);
        result += ss.str();

        // Process the rest of the format string with remaining arguments
        result +=
            FormatString(format.substr(pos + 2), std::forward<Args>(args)...);

        return result;
    }

}  // namespace Engine
