#include <cmath>
#include <iostream>

#include "Core/Assert.h"
#include "Core/String/StringUtils.h"

namespace Engine {
    namespace Test {

        void RunStringUtilsTests() {
            std::cout << "\nRunning String Utils Tests:" << std::endl;

            // Test 1: Case Conversion
            {
                std::cout << "\nTesting Case Conversion:" << std::endl;

                std::string test = "Hello World";
                std::string upper = StringUtils::ToUpper(test);
                std::string lower = StringUtils::ToLower(test);

                ASSERT(upper == "HELLO WORLD", "ToUpper failed");
                ASSERT(lower == "hello world", "ToLower failed");

                std::cout << "Case conversion tests passed!" << std::endl;
            }

            // Test 2: Trimming
            {
                std::cout << "\nTesting String Trimming:" << std::endl;

                std::string test = "  Hello World  ";
                std::string trimLeft = StringUtils::TrimLeft(test);
                std::string trimRight = StringUtils::TrimRight(test);
                std::string trimBoth = StringUtils::Trim(test);

                ASSERT(trimLeft == "Hello World  ", "TrimLeft failed");
                ASSERT(trimRight == "  Hello World", "TrimRight failed");
                ASSERT(trimBoth == "Hello World", "Trim failed");

                std::cout << "Trimming tests passed!" << std::endl;
            }

            // Test 3: Splitting and Joining
            {
                std::cout << "\nTesting Split and Join:" << std::endl;

                std::string test = "one,two,three,four";
                auto parts = StringUtils::Split(test, ',');

                ASSERT(parts.size() == 4, "Split count incorrect");
                ASSERT(parts[0] == "one", "Split first element incorrect");
                ASSERT(parts[3] == "four", "Split last element incorrect");

                std::string joined = StringUtils::Join(parts, "-");
                ASSERT(joined == "one-two-three-four", "Join failed");

                // Test split with string delimiter
                std::string test2 = "one::two::three::four";
                auto parts2 = StringUtils::Split(test2, "::");
                ASSERT(parts2.size() == 4,
                       "Split with string delimiter failed");

                std::cout << "Split and join tests passed!" << std::endl;
            }

            // Test 4: String Replacement
            {
                std::cout << "\nTesting String Replacement:" << std::endl;

                std::string test = "Hello World, Hello Everyone";
                std::string replaced =
                    StringUtils::Replace(test, "Hello", "Hi");

                ASSERT(replaced == "Hi World, Hi Everyone", "Replace failed");

                std::cout << "Replacement tests passed!" << std::endl;
            }

            // Test 5: Prefix and Suffix Checks
            {
                std::cout << "\nTesting Prefix and Suffix Checks:" << std::endl;

                std::string test = "HelloWorld";

                ASSERT(StringUtils::StartsWith(test, "Hello"),
                       "StartsWith failed");
                ASSERT(!StringUtils::StartsWith(test, "World"),
                       "StartsWith negative test failed");
                ASSERT(StringUtils::EndsWith(test, "World"), "EndsWith failed");
                ASSERT(!StringUtils::EndsWith(test, "Hello"),
                       "EndsWith negative test failed");

                std::cout << "Prefix and suffix tests passed!" << std::endl;
            }

            // Test 6: String Type Checking
            {
                std::cout << "\nTesting String Type Checking:" << std::endl;

                ASSERT(StringUtils::IsNumeric("12345"),
                       "IsNumeric positive test failed");
                ASSERT(!StringUtils::IsNumeric("12a45"),
                       "IsNumeric negative test failed");

                ASSERT(StringUtils::IsAlpha("abcde"),
                       "IsAlpha positive test failed");
                ASSERT(!StringUtils::IsAlpha("abc123"),
                       "IsAlpha negative test failed");

                ASSERT(StringUtils::IsAlphaNumeric("abc123"),
                       "IsAlphaNumeric positive test failed");
                ASSERT(!StringUtils::IsAlphaNumeric("abc 123"),
                       "IsAlphaNumeric negative test failed");

                std::cout << "String type checking tests passed!" << std::endl;
            }

            // Test 7: Case Conversion
            {
                std::cout << "\nTesting Case Style Conversion:" << std::endl;

                std::string snake = "hello_world_test";
                std::string camel = StringUtils::ToCamelCase(snake);
                ASSERT(camel == "helloWorldTest", "ToCamelCase failed");

                std::string backToSnake = StringUtils::ToSnakeCase(camel);
                ASSERT(backToSnake == "hello_world_test", "ToSnakeCase failed");

                std::cout << "Case style conversion tests passed!" << std::endl;
            }

            // Test 8: String Hashing
            {
                std::cout << "\nTesting String Hashing:" << std::endl;

                std::string test1 = "Hello";
                std::string test2 = "World";

                size_t hash1 = StringUtils::Hash(test1);
                size_t hash2 = StringUtils::Hash(test2);
                size_t hash1Repeat = StringUtils::Hash(test1);

                ASSERT(hash1 != hash2,
                       "Different strings should have different hashes");
                ASSERT(hash1 == hash1Repeat,
                       "Same string should have same hash");

                std::cout << "String hashing tests passed!" << std::endl;
            }

            // Test 9: Base64 Encoding/Decoding
            {
                std::cout << "\nTesting Base64 Encoding/Decoding:" << std::endl;

                std::string original = "Hello, World! 123";
                std::string encoded = StringUtils::Base64Encode(original);
                std::string decoded = StringUtils::Base64Decode(encoded);

                ASSERT(decoded == original,
                       "Base64 encode/decode round trip failed");

                std::cout << "Base64 encoding/decoding tests passed!"
                          << std::endl;
            }

            // Test 10: String Conversion
            {
                std::cout << "\nTesting String Conversion:" << std::endl;

                // Test integer conversion
                int testInt = 12345;
                std::string intStr = StringUtils::ToString(testInt);
                ASSERT(intStr == "12345", "Int to string conversion failed");

                int parsedInt = StringUtils::FromString<int>("12345");
                ASSERT(parsedInt == 12345, "String to int conversion failed");

                // Test float conversion
                float testFloat = 123.45f;
                std::string floatStr = StringUtils::ToString(testFloat);
                float parsedFloat = StringUtils::FromString<float>(floatStr);
                ASSERT(std::abs(parsedFloat - testFloat) < 0.0001f,
                       "Float conversion failed");

                std::cout << "String conversion tests passed!" << std::endl;
            }

            // Test 11: String Formatting
            {
                std::cout << "\nTesting String Formatting:" << std::endl;

                std::string formatted = StringUtils::Format(
                    "Hello {}, your number is {}", "John", 42);
                ASSERT(formatted == "Hello John, your number is 42",
                       "String formatting failed");

                formatted =
                    StringUtils::Format("Pi is approximately {}", 3.14159f);
                ASSERT(formatted.find("3.14159") != std::string::npos,
                       "Float formatting failed");

                std::cout << "String formatting tests passed!" << std::endl;
            }

            // Test 12: Wide String Conversion
            {
                std::cout << "\nTesting Wide String Conversion:" << std::endl;

                std::string narrow = "Hello, World!";
                std::wstring wide = StringUtils::ToWideString(narrow);
                std::string backToNarrow = StringUtils::ToString(wide);

                ASSERT(backToNarrow == narrow,
                       "Wide string conversion round trip failed");

                std::cout << "Wide string conversion tests passed!"
                          << std::endl;
            }

            std::cout << "\nAll string utils tests completed successfully!"
                      << std::endl;
        }

    }  // namespace Test
}  // namespace Engine

// Add main function
int main() {
    try {
        Engine::Test::RunStringUtilsTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
