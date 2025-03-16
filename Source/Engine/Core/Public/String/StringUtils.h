#pragma once

#include <cstddef>  // For size_t
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "CoreTypes.h"

namespace Engine {
    class StringUtils {
      public:
        static bool ToBool(const std::string &str) {
            std::string lower = ToLower(str);
            return lower == "true" || lower == "1" || lower == "yes" ||
                   lower == "on";
        }

        // Case conversion
        static std::string ToLower(const std::string &str);
        static std::string ToUpper(const std::string &str);
        static bool EqualsIgnoreCase(const std::string &a,
                                     const std::string &b) {
            if (a.length() != b.length()) return false;
            for (size_t i = 0; i < a.length(); ++i) {
                if (std::tolower(a[i]) != std::tolower(b[i])) return false;
            }
            return true;
        }

        // String conversion methods
        template <typename T>
        static T FromString(const std::string &str) {
            T value;
            std::istringstream iss(str);
            iss >> value;
            return value;
        }

        template <typename T>
        static std::string ToString(const T &value) {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }

        // Specialized conversion for std::wstring
        static std::string ToString(const std::wstring &wstr);
        static std::wstring ToWideString(const std::string &str);

        // Trimming
        static std::string Trim(const std::string &str);
        static std::string TrimLeft(const std::string &str);
        static std::string TrimRight(const std::string &str);

        // Splitting/Joining
        static std::vector<std::string> Split(const std::string &str,
                                              char delimiter);
        static std::vector<std::string> Split(const std::string &str,
                                              const std::string &delimiter);
        static std::string Join(const std::vector<std::string> &strings,
                                const std::string &delimiter);
        static std::vector<std::string> Split(std::string_view str,
                                              std::string_view delimiter,
                                              bool keepEmpty = false);
        static std::string Join(std::string_view delimiter,
                                const std::string *begin,
                                const std::string *end);

        // String checks
        static bool StartsWith(const std::string &str,
                               const std::string &prefix);
        static bool EndsWith(const std::string &str, const std::string &suffix);
        static bool IsNumeric(const std::string &str);
        static bool IsAlpha(const std::string &str);
        static bool IsAlphaNumeric(const std::string &str);

        // String manipulation
        static std::string Replace(const std::string &str,
                                   const std::string &from,
                                   const std::string &to,
                                   size_t maxReplacements = 100);
        static std::string ReplaceAll(std::string_view str,
                                      std::string_view from,
                                      std::string_view to,
                                      size_t maxReplacements = 100);
        template <typename... Args>
        static std::string Format(const char *format, Args &&...args) {
            std::ostringstream oss;
            FormatImpl(oss, format, std::forward<Args>(args)...);
            return oss.str();
        }

        // Encoding/Conversion
        static std::string Base64Encode(const std::string &str);
        static std::string Base64Decode(const std::string &str);
        static std::string UrlEncode(std::string_view str);
        static std::string UrlDecode(std::string_view str);

        // Case formatting
        static std::string ToCamelCase(const std::string &str);
        static std::string ToSnakeCase(const std::string &str);

        // Hashing
        static size_t Hash(const std::string &str);

        // String generation
        static std::string Repeat(std::string_view pattern, int count);
        static std::string Random(size_t length,
                                  std::string_view charSet =
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmn"
                                      "opqrstuvwxyz0123456789");

        // String transformation
        static std::string Reverse(std::string_view str);
        static std::string PadLeft(std::string_view str,
                                   size_t totalWidth,
                                   char paddingChar = ' ');
        static std::string PadRight(std::string_view str,
                                    size_t totalWidth,
                                    char paddingChar = ' ');
        static std::string Remove(std::string_view str,
                                  std::string_view removeStr);
        static std::string RemoveChars(std::string_view str,
                                       std::string_view charsToRemove);

        // String queries
        static bool Contains(std::string_view str,
                             std::string_view value,
                             bool caseSensitive = true);
        static int Count(std::string_view str, std::string_view substr);
        static std::vector<size_t> AllIndicesOf(std::string_view str,
                                                std::string_view substr);

        // String validation
        static bool IsWhitespace(std::string_view str);
        static bool IsValidEmail(std::string_view email);
        static bool IsValidIPAddress(std::string_view ip);

        // Encoding conversion
        static std::string HtmlEncode(std::string_view str);
        static std::string HtmlDecode(std::string_view str);

        // Path processing
        static std::string GetFileExtension(std::string_view path);
        static std::string GetFileNameWithoutExtension(std::string_view path);
        static std::string CombinePath(std::string_view path1,
                                       std::string_view path2);

        // Advanced split/join
        static std::vector<std::string> SplitLines(std::string_view str,
                                                   bool keepEndings = false);
        static std::string Join(
            const std::initializer_list<std::string_view> &strings,
            std::string_view delimiter);

        // Template expansion
        template <typename... Args>
        static std::string Concat(const Args &...args) {
            std::ostringstream oss;
            (oss << ... << args);  // C++17 fold expression
            return oss.str();
        }

        // Unicode support
        static std::u16string ToUTF16(std::string_view utf8Str);
        static std::u32string ToUTF32(std::string_view utf8Str);
        static std::string FromUTF16(std::u16string_view utf16Str);
        static std::string FromUTF32(std::u32string_view utf32Str);

      private:
        // Private constructor and destructor to prevent instantiation
        StringUtils() = delete;
        ~StringUtils() = delete;

        static bool IsNullOrEmpty(const std::string &str);
        static bool IsNullOrWhitespace(const std::string &str);

        // Wildcard matching (supports ? and *)
        static bool WildcardMatch(std::string_view str,
                                  std::string_view pattern);

        // Calculate Levenshtein edit distance
        static int LevenshteinDistance(std::string_view s1,
                                       std::string_view s2);

        // Regex operations
        static std::string RegexReplace(std::string_view str,
                                        std::string_view pattern,
                                        std::string_view replacement);

        static bool RegexMatch(std::string_view str, std::string_view pattern);
        bool RegexMatch(std::string_view str,
                        std::string_view pattern,
                        std::string *errorMsg);

        // Extract content between two tokens
        static std::string Between(std::string_view str,
                                   std::string_view start,
                                   std::string_view end);

        // Version number comparison (returns -1/0/1)
        static int CompareVersions(std::string_view v1, std::string_view v2);

        // Smart truncation with ellipsis
        static std::string TruncateWithEllipsis(std::string_view str,
                                                size_t maxLength,
                                                bool keepWord = true);

        // String normalization (remove accent marks etc.)
        static std::string NormalizeString(std::string_view str);

        // Count character occurrences
        static int CountChar(std::string_view str, char ch);

        // Generate N-gram set from string
        static std::set<std::string> GenerateNGrams(std::string_view str,
                                                    unsigned int n);

      private:  // Format implementation details
        static void FormatImpl(std::ostringstream &oss, const char *format) {
            oss << format;
        }

        template <typename T, typename... Args>
        static void FormatImpl(std::ostringstream &oss,
                               const char *format,
                               T &&value,
                               Args &&...args) {
            for (; *format != '\0'; ++format) {
                if (*format == '{' && *(format + 1) == '}') {
                    oss << ValueToString(std::forward<T>(value));
                    FormatImpl(oss, format + 2, std::forward<Args>(args)...);
                    return;
                }
                oss << *format;
            }
        }

        // Type conversion handlers
        template <typename T>
        static std::string ValueToString(T &&value) {
            if constexpr (std::is_same_v<
                              std::remove_cv_t<std::remove_reference_t<T>>,
                              std::string>) {
                return std::forward<T>(value);
            } else if constexpr (
                std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                               const char *> ||
                std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                               char *>) {
                return std::string(value);
            } else if constexpr (std::is_arithmetic_v<std::remove_cv_t<
                                     std::remove_reference_t<T>>>) {
                return std::to_string(value);
            } else if constexpr (std::is_convertible_v<T, std::string>) {
                return std::string(std::forward<T>(value));
            } else {
                std::ostringstream oss;
                oss << value;
                return oss.str();
            }
        }

        static std::string ValueToString(const std::string &value) {
            return value;
        }

        static std::string ValueToString(const char *value) { return value; }
    };

    // Template specializations for common types
    template <>
    inline int StringUtils::FromString<int>(const std::string &str) {
        return std::stoi(str);
    }

    template <>
    inline float StringUtils::FromString<float>(const std::string &str) {
        return std::stof(str);
    }

    template <>
    inline double StringUtils::FromString<double>(const std::string &str) {
        return std::stod(str);
    }

    template <>
    inline bool StringUtils::FromString<bool>(const std::string &str) {
        return ToBool(str);
    }

}  // namespace Engine
