
#include "String/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <locale>
#include <random>
#include <regex>
#include <sstream>
#include <variant>
namespace Engine {

    std::string StringUtils::ToUpper(const std::string &str) {
        std::string result = str;
        std::transform(
            result.begin(), result.end(), result.begin(), [](unsigned char c) {
                return std::toupper(c);
            });
        return result;
    }

    std::string StringUtils::Trim(const std::string &str) {
        if (str.empty()) {
            return str;
        }

        const auto begin = str.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            return "";
        }

        const auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(begin, end - begin + 1);
    }

    std::string StringUtils::TrimLeft(const std::string &str) {
        if (str.empty()) {
            return str;
        }

        const auto begin = str.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            return "";
        }

        return str.substr(begin);
    }

    std::string StringUtils::TrimRight(const std::string &str) {
        if (str.empty()) {
            return str;
        }

        const auto end = str.find_last_not_of(" \t\r\n");
        if (end == std::string::npos) {
            return "";
        }

        return str.substr(0, end + 1);
    }

    std::vector<std::string> StringUtils::Split(const std::string &str,
                                                char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }

        return tokens;
    }

    std::vector<std::string> StringUtils::Split(const std::string &str,
                                                const std::string &delimiter) {
        std::vector<std::string> tokens;
        if (delimiter.empty()) {
            tokens.push_back(str);
            return tokens;
        }

        size_t pos = 0, prev = 0;
        const size_t delim_len = delimiter.length();

        // 预分配空间优化
        tokens.reserve(str.length() / (delim_len + 1) + 1);

        while ((pos = str.find(delimiter, prev)) != std::string::npos) {
            if (pos != prev) {  // 跳过空token
                tokens.emplace_back(str.substr(prev, pos - prev));
            }
            prev = pos + delim_len;
        }

        if (prev <= str.length()) {
            tokens.emplace_back(str.substr(prev));
        }

        return tokens;
    }

    // 增强版Split（支持保留空token和多种分割方式）
    std::vector<std::string> StringUtils::Split(std::string_view str,
                                                std::string_view delimiter,
                                                bool keepEmpty) {
        std::vector<std::string> tokens;
        if (delimiter.empty()) {
            tokens.emplace_back(str);
            return tokens;
        }

        size_t pos = 0, prev = 0;
        const size_t delimLen = delimiter.length();

        // 预分配内存优化
        tokens.reserve(str.length() / (delimLen + 1) + 1);

        while ((pos = str.find(delimiter, prev)) != std::string_view::npos) {
            if (keepEmpty || (pos != prev)) {
                tokens.emplace_back(str.substr(prev, pos - prev));
            }
            prev = pos + delimLen;
        }

        // 处理最后一个token
        if (prev <= str.length()) {
            std::string_view last = str.substr(prev);
            if (keepEmpty || !last.empty()) {
                tokens.emplace_back(last);
            }
        }

        return tokens;
    }

    std::string StringUtils::Join(const std::vector<std::string> &strings,
                                  const std::string &delimiter) {
        if (strings.empty()) {
            return "";
        }

        std::ostringstream result;
        result << strings[0];

        for (size_t i = 1; i < strings.size(); ++i) {
            result << delimiter << strings[i];
        }

        return result.str();
    }

    // 高性能Join实现（迭代器版本）
    std::string StringUtils::Join(std::string_view delimiter,
                                  const std::string *begin,
                                  const std::string *end) {
        if (begin == end) return "";

        size_t totalLen = 0;
        const size_t delimLen = delimiter.length();
        const std::string *it = begin;

        // 计算总长度
        do {
            totalLen += it->length();
        } while (++it != end && (totalLen += delimLen));

        // 预分配内存
        std::string result;
        result.reserve(totalLen);

        // 拼接字符串
        it = begin;
        result.append(*it);
        while (++it != end) {
            result.append(delimiter).append(*it);
        }

        return result;
    }

    bool StringUtils::StartsWith(const std::string &str,
                                 const std::string &prefix) {
        if (str.length() < prefix.length()) {
            return false;
        }

        return str.compare(0, prefix.length(), prefix) == 0;
    }

    bool StringUtils::EndsWith(const std::string &str,
                               const std::string &suffix) {
        if (str.length() < suffix.length()) {
            return false;
        }

        return str.compare(str.length() - suffix.length(),
                           suffix.length(),
                           suffix) == 0;
    }

    // 优化后的Replace实现（支持最大替换次数）
    std::string StringUtils::Replace(const std::string &str,
                                     const std::string &from,
                                     const std::string &to,
                                     size_t maxReplacements) {
        if (from.empty()) return std::string(str);

        std::string result;
        size_t startPos = 0;
        size_t count = 0;
        const size_t fromLen = from.length();
        [[maybe_unused]] const size_t toLen = to.length();

        // 预分配内存（假设至少替换一次）
        result.reserve(str.length() + 32);

        while (count != maxReplacements) {
            size_t found = str.find(from, startPos);
            if (found == std::string::npos) break;

            result.append(str, startPos, found - startPos);
            result.append(to);
            startPos = found + fromLen;
            ++count;
        }

        result.append(str.substr(startPos));
        return result;
    }

    // 使用平台特定实现替代std::wstring_convert
    std::wstring StringUtils::ToWideString(const std::string &str) {
#if defined(_WIN32)
        if (str.empty()) return L"";
        int size_needed = MultiByteToWideChar(
            CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(
            CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
        return wstr;
#else
        // Linux/Mac使用iconv或其他跨平台方案
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
#endif
    }

    std::string StringUtils::ToString(const std::wstring &wstr) {
#if defined(_WIN32)
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8,
                                              0,
                                              wstr.c_str(),
                                              (int)wstr.size(),
                                              nullptr,
                                              0,
                                              nullptr,
                                              nullptr);
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8,
                            0,
                            wstr.c_str(),
                            (int)wstr.size(),
                            &str[0],
                            size_needed,
                            nullptr,
                            nullptr);
        return str;
#else
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wstr);
#endif
    }

    // 增加字符类型检查
    bool StringUtils::IsNumeric(const std::string &str) {
        return !str.empty() &&
               std::all_of(str.begin(), str.end(), [](unsigned char c) {
                   return std::isdigit(c);
               });
    }

    // 正确处理UTF-8字符的大小写转换
    std::string StringUtils::ToLower(const std::string &str) {
        std::string result;
        result.reserve(str.size());
        std::locale loc("");
        for (unsigned char c : str) {
            result += std::tolower(c, loc);
        }
        return result;
    }

    bool StringUtils::IsAlpha(const std::string &str) {
        return !str.empty() && std::all_of(str.begin(), str.end(), ::isalpha);
    }

    bool StringUtils::IsAlphaNumeric(const std::string &str) {
        return !str.empty() && std::all_of(str.begin(), str.end(), ::isalnum);
    }

    std::string StringUtils::ToCamelCase(const std::string &str) {
        std::string result;
        bool capitalizeNext = false;

        for (char c : str) {
            if (c == '_' || c == ' ' || c == '-') {
                capitalizeNext = true;
            } else {
                if (capitalizeNext) {
                    result += static_cast<char>(std::toupper(c));
                    capitalizeNext = false;
                } else {
                    result += static_cast<char>(std::tolower(c));
                }
            }
        }

        return result;
    }

    std::string StringUtils::ToSnakeCase(const std::string &str) {
        std::string result;

        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];
            if (i > 0 && std::isupper(c)) {
                result += '_';
            }
            result += static_cast<char>(std::tolower(c));
        }

        return result;
    }

    size_t StringUtils::Hash(const std::string &str) {
        // FNV-1a hash algorithm
        static const size_t fnvOffsetBasis = 14695981039346656037ULL;
        static const size_t fnvPrime = 1099511628211ULL;

        size_t hash = fnvOffsetBasis;

        for (char c : str) {
            hash ^= static_cast<size_t>(c);
            hash *= fnvPrime;
        }

        return hash;
    }

    std::string StringUtils::Base64Encode(const std::string &str) {
        static const std::string base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        int val = 0;
        int valb = -6;

        for (unsigned char c : str) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(base64Chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) {
            result.push_back(base64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }

        while (result.size() % 4) {
            result.push_back('=');
        }

        return result;
    }

    std::string StringUtils::Base64Decode(const std::string &str) {
        // 预计算解码后大小
        size_t decoded_length = (str.size() * 3) / 4;
        if (str.size() >= 2) {
            if (str[str.size() - 1] == '=') decoded_length--;
            if (str[str.size() - 2] == '=') decoded_length--;
        }

        std::string result;
        result.reserve(decoded_length);

        // 重构解码逻辑
        uint32_t val = 0;
        int bits = 0;
        for (unsigned char c : str) {
            if (c == '=') break;  // 提前终止

            int64_t value = -1;
            if (c >= 'A' && c <= 'Z')
                value = c - 'A';
            else if (c >= 'a' && c <= 'z')
                value = 26 + c - 'a';
            else if (c >= '0' && c <= '9')
                value = 52 + c - '0';
            else if (c == '+')
                value = 62;
            else if (c == '/')
                value = 63;
            if (value == -1) continue;  // 跳过无效字符

            val = (val << 6) | value;
            bits += 6;

            if (bits >= 8) {
                bits -= 8;
                result += static_cast<char>((val >> bits) & 0xFF);
            }
        }

        return result;
    }

    bool NeedsUrlEncoding(char c) {
        // RFC 3986 section 2.3 Unreserved Characters
        return !(std::isalnum(static_cast<unsigned char>(c)) || c == '-' ||
                 c == '_' || c == '.' || c == '~');
    }

    // Helper function to convert a hex character to its decimal value
    int HexToInt(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    std::string StringUtils::UrlEncode(std::string_view str) {
        std::ostringstream encoded;
        encoded.fill('0');
        encoded << std::hex;

        for (char c : str) {
            if (NeedsUrlEncoding(c)) {
                encoded << '%' << std::setw(2)
                        << static_cast<int>(static_cast<unsigned char>(c));
            } else {
                encoded << c;
            }
        }

        return encoded.str();
    }

    std::string StringUtils::UrlDecode(std::string_view str) {
        std::string decoded;
        decoded.reserve(str.length());

        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%') {
                if (i + 2 >= str.length()) {  // 不完整编码
                    decoded += str[i];
                    continue;
                }

                int high = HexToInt(str[i + 1]);
                int low = HexToInt(str[i + 2]);

                if (high != -1 && low != -1) {
                    decoded += static_cast<char>((high << 4) | low);
                    i += 2;
                } else {
                    // 保留无效编码
                    decoded.append(&str[i], 3);
                    i += 2;
                }
            } else if (str[i] == '+') {  // 处理+号空格
                decoded += ' ';
            } else {
                decoded += str[i];
            }
        }

        return decoded;
    }

// Add any platform-specific implementations here if needed
#if PLATFORM_WINDOWS
    const char *stristr(const char *str1, const char *str2) {
        const char *cp = str1;
        const char *s1, *s2;

        if (!*str2) return str1;

        while (*cp) {
            s1 = cp;
            s2 = str2;

            while (*s1 && *s2 && !(tolower(*s1) - tolower(*s2))) {
                s1++;
                s2++;
            }

            if (!*s2) return cp;
            cp++;
        }

        return nullptr;
    }
#endif

    std::string StringUtils::Repeat(std::string_view pattern, int count) {
        if (count <= 0 || pattern.empty()) {
            return "";
        }
        std::string result;
        result.reserve(pattern.length() * count);
        for (int i = 0; i < count; ++i) {
            result.append(pattern);
        }
        return result;
    }

    std::string StringUtils::Random(size_t length, std::string_view charSet) {
        if (length == 0 || charSet.empty()) {
            return "";
        }

        std::string result;
        result.reserve(length);

        // 使用随机设备作为种子
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, charSet.length() - 1);

        for (size_t i = 0; i < length; ++i) {
            result += charSet[dist(gen)];
        }

        return result;
    }

    std::string StringUtils::Reverse(std::string_view str) {
        std::string result(str);
        std::reverse(result.begin(), result.end());
        return result;
    }

    std::string StringUtils::PadLeft(std::string_view str,
                                     size_t totalWidth,
                                     char paddingChar) {
        if (str.length() >= totalWidth) {
            return std::string(str);
        }
        return std::string(totalWidth - str.length(), paddingChar) +
               std::string(str);
    }

    std::string StringUtils::PadRight(std::string_view str,
                                      size_t totalWidth,
                                      char paddingChar) {
        if (str.length() >= totalWidth) {
            return std::string(str);
        }
        return std::string(str) +
               std::string(totalWidth - str.length(), paddingChar);
    }

    bool StringUtils::IsNullOrEmpty(const std::string &str) {
        return str.empty();
    }

    bool StringUtils::IsNullOrWhitespace(const std::string &str) {
        return str.empty() ||
               std::all_of(str.begin(), str.end(), [](unsigned char c) {
                   return std::isspace(c);
               });
    }

    std::string StringUtils::Remove(std::string_view str,
                                    std::string_view removeStr) {
        if (str.empty() || removeStr.empty()) {
            return std::string(str);
        }

        std::string result(str);
        size_t pos = 0;
        while ((pos = result.find(removeStr, pos)) != std::string::npos) {
            result.erase(pos, removeStr.length());
        }
        return result;
    }

    std::string StringUtils::RemoveChars(std::string_view str,
                                         std::string_view charsToRemove) {
        if (str.empty() || charsToRemove.empty()) {
            return std::string(str);
        }

        std::string result(str);
        for (char c : charsToRemove) {
            result.erase(std::remove(result.begin(), result.end(), c),
                         result.end());
        }
        return result;
    }

    bool StringUtils::Contains(std::string_view str,
                               std::string_view value,
                               bool caseSensitive) {
        if (str.empty() || value.empty()) {
            return false;
        }

        if (caseSensitive) {
            return str.find(value) != std::string_view::npos;
        }

        // 不区分大小写的搜索
        std::string lowerStr = ToLower(std::string(str));
        std::string lowerValue = ToLower(std::string(value));
        return lowerStr.find(lowerValue) != std::string::npos;
    }

    int StringUtils::Count(std::string_view str, std::string_view substr) {
        if (str.empty() || substr.empty()) {
            return 0;
        }

        int count = 0;
        size_t pos = 0;
        while ((pos = str.find(substr, pos)) != std::string_view::npos) {
            ++count;
            pos += substr.length();
        }
        return count;
    }

    std::vector<size_t> StringUtils::AllIndicesOf(std::string_view str,
                                                  std::string_view substr) {
        std::vector<size_t> indices;
        if (str.empty() || substr.empty()) {
            return indices;
        }

        size_t pos = 0;
        while ((pos = str.find(substr, pos)) != std::string_view::npos) {
            indices.push_back(pos);
            pos += substr.length();
        }
        return indices;
    }

    bool StringUtils::IsWhitespace(std::string_view str) {
        return std::all_of(str.begin(), str.end(), [](unsigned char c) {
            return std::isspace(c);
        });
    }

    bool StringUtils::IsValidEmail(std::string_view email) {
        // 基本的电子邮件格式验证
        if (email.empty() || email.length() > 254) {
            return false;
        }

        // 查找@符号
        size_t atPos = email.find('@');
        if (atPos == std::string_view::npos || atPos == 0 ||
            atPos == email.length() - 1) {
            return false;
        }

        // 检查本地部分和域名部分
        std::string_view localPart = email.substr(0, atPos);
        std::string_view domainPart = email.substr(atPos + 1);

        // 验证本地部分
        if (localPart.empty() || localPart.length() > 64) {
            return false;
        }

        // 验证域名部分
        if (domainPart.empty() || domainPart.length() > 255) {
            return false;
        }

        // 检查基本字符有效性
        return std::all_of(email.begin(), email.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '@' || c == '.' || c == '_' ||
                   c == '-';
        });
    }

    bool StringUtils::IsValidIPAddress(std::string_view ip) {
        if (ip.empty()) {
            return false;
        }

        // 分割IP地址
        std::vector<std::string> parts;
        std::string ipStr(ip);
        std::stringstream ss(ipStr);
        std::string segment;

        while (std::getline(ss, segment, '.')) {
            parts.push_back(segment);
        }

        // 检查是否正好有4个部分
        if (parts.size() != 4) {
            return false;
        }

        // 验证每个部分
        for (const auto &part : parts) {
            // 检查长度和前导零
            if (part.empty() || (part.length() > 1 && part[0] == '0')) {
                return false;
            }

            // 检查是否都是数字
            if (!std::all_of(part.begin(), part.end(), ::isdigit)) {
                return false;
            }

            // 转换为数字并检查范围
            try {
                int value = std::stoi(part);
                if (value < 0 || value > 255) {
                    return false;
                }
            } catch (...) {
                return false;
            }
        }

        return true;
    }

    std::string StringUtils::HtmlEncode(std::string_view str) {
        std::string result;
        result.reserve(str.length());

        for (char c : str) {
            switch (c) {
                case '&':
                    result.append("&amp;");
                    break;
                case '<':
                    result.append("&lt;");
                    break;
                case '>':
                    result.append("&gt;");
                    break;
                case '"':
                    result.append("&quot;");
                    break;
                case '\'':
                    result.append("&#39;");
                    break;
                default:
                    result += c;
                    break;
            }
        }

        return result;
    }

    std::string StringUtils::HtmlDecode(std::string_view str) {
        std::string result;
        result.reserve(str.length());

        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '&') {
                std::string_view entity;
                size_t end = str.find(';', i);

                if (end != std::string_view::npos) {
                    entity = str.substr(i, end - i + 1);

                    if (entity == "&amp;") {
                        result += '&';
                        i = end;
                    } else if (entity == "&lt;") {
                        result += '<';
                        i = end;
                    } else if (entity == "&gt;") {
                        result += '>';
                        i = end;
                    } else if (entity == "&quot;") {
                        result += '"';
                        i = end;
                    } else if (entity == "&#39;") {
                        result += '\'';
                        i = end;
                    } else {
                        result += str[i];
                    }
                } else {
                    result += str[i];
                }
            } else {
                result += str[i];
            }
        }

        return result;
    }

    std::string StringUtils::GetFileExtension(std::string_view path) {
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string_view::npos || dotPos == 0) {
            return "";
        }

        // 检查是否在最后一个路径分隔符之后
        size_t lastSeparator = path.find_last_of("/\\");
        if (lastSeparator != std::string_view::npos && dotPos < lastSeparator) {
            return "";
        }

        return std::string(path.substr(dotPos));
    }

    std::string StringUtils::GetFileNameWithoutExtension(
        std::string_view path) {
        // 找到最后一个路径分隔符
        size_t lastSeparator = path.find_last_of("/\\");
        size_t startPos =
            (lastSeparator == std::string_view::npos) ? 0 : lastSeparator + 1;

        // 找到最后一个点号
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string_view::npos || dotPos < startPos) {
            // 如果没有扩展名或点号在文件名之前，返回整个文件名
            return std::string(path.substr(startPos));
        }

        return std::string(path.substr(startPos, dotPos - startPos));
    }

    std::string StringUtils::CombinePath(std::string_view path1,
                                         std::string_view path2) {
        if (path1.empty()) {
            return std::string(path2);
        }
        if (path2.empty()) {
            return std::string(path1);
        }

        std::string result;
        result.reserve(path1.length() + path2.length() + 1);

        result = path1;

        // 处理路径分隔符
        char lastChar = path1.back();
        char firstChar = path2.front();

        if (lastChar == '/' || lastChar == '\\') {
            if (firstChar == '/' || firstChar == '\\') {
                result.append(path2.substr(1));
            } else {
                result.append(path2);
            }
        } else {
            if (firstChar == '/' || firstChar == '\\') {
                result.append(path2);
            } else {
                result += '/';
                result.append(path2);
            }
        }

        // 标准化路径分隔符
        std::replace(result.begin(), result.end(), '\\', '/');

        return result;
    }

    std::vector<std::string> StringUtils::SplitLines(std::string_view str,
                                                     bool keepEndings) {
        std::vector<std::string> lines;
        size_t start = 0;
        size_t pos = 0;

        while ((pos = str.find_first_of("\r\n", start)) !=
               std::string_view::npos) {
            size_t lineEnd = pos;
            size_t nextStart = pos + 1;

            // 处理 \r\n 的情况
            if (str[pos] == '\r' && pos + 1 < str.length() &&
                str[pos + 1] == '\n') {
                if (keepEndings) {
                    lineEnd = pos + 2;
                }
                nextStart = pos + 2;
            } else if (keepEndings) {
                lineEnd = pos + 1;
            }

            lines.emplace_back(str.substr(start, lineEnd - start));
            start = nextStart;
        }

        // 添加最后一行
        if (start < str.length()) {
            lines.emplace_back(str.substr(start));
        }

        return lines;
    }

    std::string StringUtils::Join(
        const std::initializer_list<std::string_view> &strings,
        std::string_view delimiter) {
        if (strings.size() == 0) {
            return "";
        }

        // 计算所需的总容量
        size_t totalLength = 0;
        for (const auto &str : strings) {
            totalLength += str.length();
        }
        totalLength += delimiter.length() * (strings.size() - 1);

        // 预分配空间
        std::string result;
        result.reserve(totalLength);

        // 添加第一个字符串
        auto it = strings.begin();
        result.append(*it);

        // 添加其余的字符串
        for (++it; it != strings.end(); ++it) {
            result.append(delimiter);
            result.append(*it);
        }

        return result;
    }
#define UCHAR_TYPE uint16_t
#include <unicode/ucnv.h>
#include <unicode/ustring.h>
    std::u16string StringUtils::ToUTF16(std::string_view utf8Str) {
        std::u16string utf16Str;
        UErrorCode status = U_ZERO_ERROR;

        // 预转换获取所需长度
        int32_t length = 0;
        u_strFromUTF8(
            nullptr, 0, &length, utf8Str.data(), utf8Str.size(), &status);
        if (status != U_BUFFER_OVERFLOW_ERROR) {
            return utf16Str;
        }

        utf16Str.resize(length);
        status = U_ZERO_ERROR;
        u_strFromUTF8(reinterpret_cast<UChar *>(utf16Str.data()),
                      utf16Str.capacity(),
                      &length,
                      utf8Str.data(),
                      utf8Str.size(),
                      &status);

        if (U_FAILURE(status)) {
            utf16Str.clear();
        } else {
            utf16Str.resize(length);
        }

        return utf16Str;
    }

    std::u32string StringUtils::ToUTF32(std::string_view utf8Str) {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        try {
            return converter.from_bytes(utf8Str.data(),
                                        utf8Str.data() + utf8Str.size());
        } catch (const std::range_error &) {
            // 处理转换错误，返回空字符串
            return std::u32string();
        }
    }

    std::string StringUtils::FromUTF16(std::u16string_view utf16Str) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>
            converter;
        try {
            return converter.to_bytes(utf16Str.data(),
                                      utf16Str.data() + utf16Str.size());
        } catch (const std::range_error &) {
            // 处理转换错误，返回空字符串
            return std::string();
        }
    }

    std::string StringUtils::FromUTF32(std::u32string_view utf32Str) {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        try {
            return converter.to_bytes(utf32Str.data(),
                                      utf32Str.data() + utf32Str.size());
        } catch (const std::range_error &) {
            // 处理转换错误，返回空字符串
            return std::string();
        }
    }

    // 增强版通配符匹配（支持转义字符）
    bool StringUtils::WildcardMatch(std::string_view str,
                                    std::string_view pattern) {
        size_t s = 0, p = 0;
        size_t star = std::string_view::npos;
        size_t match = 0;
        bool escape = false;

        while (s < str.size()) {
            if (p < pattern.size() && !escape) {
                if (pattern[p] == '\\') {
                    escape = true;
                    ++p;
                    continue;
                }

                if (pattern[p] == '*') {
                    star = p++;
                    match = s;
                    continue;
                }

                if (pattern[p] == '?') {
                    ++p;
                    ++s;
                    continue;
                }
            }

            if (p < pattern.size() && (escape || pattern[p] == str[s])) {
                ++p;
                ++s;
                escape = false;
            } else if (star != std::string_view::npos) {
                p = star + 1;
                s = ++match;
            } else {
                return false;
            }
        }

        while (p < pattern.size() && pattern[p] == '*') ++p;
        return p == pattern.size();
    }

    // 优化Levenshtein距离算法（使用滚动数组）
    int StringUtils::LevenshteinDistance(std::string_view s1,
                                         std::string_view s2) {
        const size_t m = s1.size();
        const size_t n = s2.size();

        if (m == 0) return n;
        if (n == 0) return m;

        // 使用两行滚动数组优化空间复杂度
        std::vector<int> prevRow(n + 1), currRow(n + 1);
        std::iota(prevRow.begin(), prevRow.end(), 0);

        for (size_t i = 1; i <= m; ++i) {
            currRow[0] = i;
            for (size_t j = 1; j <= n; ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                currRow[j] = std::min({
                    prevRow[j] + 1,        // 删除
                    currRow[j - 1] + 1,    // 插入
                    prevRow[j - 1] + cost  // 替换
                });
            }
            prevRow.swap(currRow);
        }

        return prevRow[n];
    }

    // 正则表达式替换
    std::string StringUtils::RegexReplace(std::string_view str,
                                          std::string_view pattern,
                                          std::string_view replacement) {
        try {
            std::regex re(pattern.begin(), pattern.end());
            std::string result = std::regex_replace(
                std::string(str), re, std::string(replacement));
            return result;
        } catch (const std::regex_error &) {
            return std::string(str);
        }
    }

    // 正则表达式匹配
    bool StringUtils::RegexMatch(std::string_view str,
                                 std::string_view pattern) {
        try {
            std::regex re(pattern.begin(), pattern.end());
            return std::regex_match(std::string(str), re);
        } catch (const std::regex_error &) {
            return false;
        }
    }

    // 增强版正则表达式匹配（带错误码）
    bool StringUtils::RegexMatch(std::string_view str,
                                 std::string_view pattern,
                                 std::string *errorMsg) {
        try {
            std::regex re(pattern.begin(),
                          pattern.end(),
                          std::regex_constants::ECMAScript |
                              std::regex_constants::optimize);
            return std::regex_match(std::string(str), re);
        } catch (const std::regex_error &e) {
            if (errorMsg) {
                std::ostringstream oss;
                oss << "Regex error (" << e.code() << "): " << e.what();
                *errorMsg = oss.str();
            }
            return false;
        }
    }

    // 提取两个标记之间的内容
    std::string StringUtils::Between(std::string_view str,
                                     std::string_view start,
                                     std::string_view end) {
        size_t startPos = str.find(start);
        if (startPos == std::string_view::npos) return "";

        startPos += start.length();
        size_t endPos = str.find(end, startPos);
        if (endPos == std::string_view::npos) return "";

        return std::string(str.substr(startPos, endPos - startPos));
    }

    // 增强版版本号比较（支持字母后缀）
    int StringUtils::CompareVersions(std::string_view v1, std::string_view v2) {
        auto splitVersion = [](std::string_view ver) {
            std::vector<std::variant<int, std::string>> parts;
            [[maybe_unused]] size_t start = 0;
            bool numeric = true;
            std::string buffer;

            auto commit = [&]() {
                if (!buffer.empty()) {
                    if (numeric) {
                        parts.emplace_back(std::stoi(buffer));
                    } else {
                        parts.emplace_back(buffer);
                    }
                    buffer.clear();
                    numeric = true;
                }
            };

            for (char c : ver) {
                if (c == '.') {
                    commit();
                } else if (std::isdigit(c)) {
                    buffer += c;
                } else {
                    if (!buffer.empty() && numeric) {
                        commit();
                    }
                    numeric = false;
                    buffer += c;
                }
            }
            commit();

            return parts;
        };

        const auto parts1 = splitVersion(v1);
        const auto parts2 = splitVersion(v2);
        const size_t maxLen = std::max(parts1.size(), parts2.size());

        for (size_t i = 0; i < maxLen; ++i) {
            const auto &p1 = (i < parts1.size()) ? parts1[i] : 0;
            const auto &p2 = (i < parts2.size()) ? parts2[i] : 0;

            if (p1.index() != p2.index()) {
                // 类型不同时，数字优先
                return (p1.index() < p2.index()) ? -1 : 1;
            }

            if (std::holds_alternative<int>(p1)) {
                const int v1 = std::get<int>(p1);
                const int v2 = std::get<int>(p2);
                if (v1 != v2) return (v1 < v2) ? -1 : 1;
            } else {
                const std::string &s1 = std::get<std::string>(p1);
                const std::string &s2 = std::get<std::string>(p2);
                const int cmp = s1.compare(s2);
                if (cmp != 0) return (cmp < 0) ? -1 : 1;
            }
        }

        return 0;
    }

    // 智能截断字符串
    std::string StringUtils::TruncateWithEllipsis(std::string_view str,
                                                  size_t maxLength,
                                                  bool keepWord) {
        if (str.length() <= maxLength) return std::string(str);

        if (maxLength < 3) return "...";

        size_t truncPos = maxLength - 3;
        if (keepWord) {
            truncPos = str.substr(0, truncPos).find_last_of(" \t\n\r");
            if (truncPos == std::string_view::npos) truncPos = maxLength - 3;
        }

        return std::string(str.substr(0, truncPos)) + "...";
    }

    // 字符统计
    int StringUtils::CountChar(std::string_view str, char ch) {
        return std::count(str.begin(), str.end(), ch);
    }

    // N-gram生成
    std::set<std::string> StringUtils::GenerateNGrams(std::string_view str,
                                                      unsigned int n) {
        std::set<std::string> ngrams;
        if (n == 0 || str.length() < n) return ngrams;

        for (size_t i = 0; i <= str.length() - n; ++i) {
            ngrams.insert(std::string(str.substr(i, n)));
        }
        return ngrams;
    }

}  // namespace Engine
