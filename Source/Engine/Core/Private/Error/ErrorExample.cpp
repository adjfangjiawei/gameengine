
#include <iostream>

#include "Error/ErrorCodes.h"
#include "Error/ErrorSystem.h"
#include "Error/Result.h"

namespace Engine {
    namespace Examples {

        // Example function that might fail
        Result<int> DivideNumbers(int a, int b) {
            if (b == 0) {
                REPORT_ERROR("Division by zero attempted",
                             ErrorSeverity::Error,
                             ErrorCategory::Custom);
                return Result<int>::Failure(ERROR_INVALID_ARGUMENT);
            }
            return Result<int>::Success(a / b);
        }

        // Example function using Result for file operations
        Result<std::string> ReadFileContent(const std::string& filename) {
            if (filename.empty()) {
                return Result<std::string>::Failure(ERROR_INVALID_ARGUMENT);
            }

            // Simulate file not found
            if (filename == "nonexistent.txt") {
                REPORT_ERROR("File not found: " + filename,
                             ErrorSeverity::Warning,
                             ErrorCategory::File);
                return Result<std::string>::Failure(ERROR_FILE_NOT_FOUND);
            }

            return Result<std::string>::Success("File content");
        }

        // Example of chaining operations
        Result<double> ProcessData(const std::string& filename, int divisor) {
            return ReadFileContent(filename)
                .AndThen([]([[maybe_unused]] const std::string& content) {
                    // Simulate some processing
                    return Result<int>::Success(100);
                })
                .AndThen([divisor](int value) {
                    return DivideNumbers(value, divisor);
                })
                .Map([](int result) {
                    return static_cast<double>(result) * 1.5;
                });
        }

        // Example of error handling with custom handler
        void SetupErrorHandler() {
            ErrorSystem::Get().RegisterHandler([](const ErrorContext& context) {
                std::cout << "Error occurred in " << context.file << ":"
                          << context.line << "\n"
                          << "Function: " << context.function << "\n"
                          << "Message: " << context.message << "\n"
                          << "Severity: " << static_cast<int>(context.severity)
                          << "\n"
                          << "Category: " << static_cast<int>(context.category)
                          << "\n"
                          << "Timestamp: " << context.timestamp << "\n\n";
            });
        }

        // Example usage demonstration
        void RunErrorHandlingDemo() {
            SetupErrorHandler();

            // Example 1: Successful operation
            auto result1 = DivideNumbers(10, 2);
            if (result1.IsSuccess()) {
                std::cout << "Division result: " << result1.GetValue()
                          << std::endl;
            }

            // Example 2: Failed operation
            auto result2 = DivideNumbers(10, 0);
            if (result2.IsFailure()) {
                std::cout << "Division failed: " << result2.GetErrorMessage()
                          << std::endl;
            }

            // Example 3: Chained operations
            ProcessData("test.txt", 2).OrElse([](ErrorCode error) {
                std::cout << "Processing failed with error: "
                          << ErrorCodeManager::Get().GetErrorMessage(error)
                          << std::endl;
                return Result<double>::Success(0.0);
            });

            // Example 4: Custom error registration
            ErrorCodeManager::Get().RegisterError(10000,
                                                  "Custom error message");

            // Example 5: Error history
            auto& errorSystem = ErrorSystem::Get();
            const auto& history = errorSystem.GetErrorHistory();
            std::cout << "Error history size: " << history.size() << std::endl;
        }

    }  // namespace Examples
}  // namespace Engine
