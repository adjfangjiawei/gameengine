#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "Error/ErrorCodes.h"
#include "Error/ErrorSystem.h"
#include "Core/Error/Result.h"

namespace Engine {
    namespace Test {

        class ErrorSystemTest {
          public:
            static void RunAllTests() {
                TestErrorSystem();
                TestErrorCodes();
                TestResult();
                TestErrorChaining();
                std::cout << "All error system tests passed successfully!\n";
            }

          private:
            static void TestErrorSystem() {
                ErrorSystem& errorSystem = ErrorSystem::Get();
                errorSystem.ClearErrors();

                // Test error reporting
                std::vector<ErrorContext> receivedErrors;
                errorSystem.RegisterHandler(
                    [&receivedErrors](const ErrorContext& context) {
                        receivedErrors.push_back(context);
                    });

                REPORT_ERROR("Test error",
                             ErrorSeverity::Warning,
                             ErrorCategory::System);
                assert(receivedErrors.size() == 1);
                assert(receivedErrors[0].message == "Test error");
                assert(receivedErrors[0].severity == ErrorSeverity::Warning);
                assert(receivedErrors[0].category == ErrorCategory::System);

                // Test error history
                const auto& history = errorSystem.GetErrorHistory();
                assert(history.size() == 1);
                assert(history[0].message == "Test error");

                // Test max history size
                errorSystem.SetMaxHistorySize(2);
                REPORT_ERROR(
                    "Error 1", ErrorSeverity::Error, ErrorCategory::Memory);
                REPORT_ERROR(
                    "Error 2", ErrorSeverity::Error, ErrorCategory::Memory);
                REPORT_ERROR(
                    "Error 3", ErrorSeverity::Error, ErrorCategory::Memory);
                assert(errorSystem.GetErrorHistory().size() == 2);
            }

            static void TestErrorCodes() {
                ErrorCodeManager& manager = ErrorCodeManager::Get();

                // Test built-in error codes
                assert(manager.IsErrorRegistered(ERROR_INVALID_ARGUMENT));
                assert(manager.GetErrorMessage(ERROR_INVALID_ARGUMENT) !=
                       "Unknown error code");

                // Test custom error registration
                const ErrorCode customError = 12345;
                const std::string customMessage = "Custom error message";
                manager.RegisterError(customError, customMessage);
                assert(manager.IsErrorRegistered(customError));
                assert(manager.GetErrorMessage(customError) == customMessage);

                // Test error categories
                assert(manager.GetErrorCategory(ERROR_MEMORY_CORRUPT) ==
                       ErrorCategory::Memory);
                assert(manager.GetErrorCategory(ERROR_FILE_NOT_FOUND) ==
                       ErrorCategory::File);
            }

            static void TestResult() {
                // Test success case
                auto successResult = Result<int>::Success(42);
                assert(successResult.IsSuccess());
                assert(!successResult.IsFailure());
                assert(successResult.GetValue() == 42);

                // Test failure case
                auto failureResult =
                    Result<int>::Failure(ERROR_INVALID_ARGUMENT);
                assert(!failureResult.IsSuccess());
                assert(failureResult.IsFailure());
                assert(failureResult.GetError() == ERROR_INVALID_ARGUMENT);

                // Test value or default
                assert(failureResult.ValueOr(100) == 100);
                assert(successResult.ValueOr(100) == 42);

                // Test void result
                auto voidSuccess = Result<void>::Success();
                assert(voidSuccess.IsSuccess());
                auto voidFailure = Result<void>::Failure(ERROR_NOT_IMPLEMENTED);
                assert(voidFailure.IsFailure());
            }

            static void TestErrorChaining() {
                // Test success chain
                auto result =
                    Result<int>::Success(10)
                        .Map([](int value) { return value * 2; })
                        .AndThen([](int value) {
                            return Result<std::string>::Success(
                                std::to_string(value));
                        })
                        .Map([](const std::string& str) { return str + "!"; });

                assert(result.IsSuccess());
                assert(result.GetValue() == "20!");

                // Test failure chain
                auto failedResult =
                    Result<int>::Failure(ERROR_INVALID_ARGUMENT)
                        .Map([](int value) { return value * 2; })
                        .AndThen([](int value) {
                            return Result<std::string>::Success(
                                std::to_string(value));
                        });

                assert(failedResult.IsFailure());
                assert(failedResult.GetError() == ERROR_INVALID_ARGUMENT);

                // Test error recovery
                auto recoveredResult =
                    failedResult.OrElse([]([[maybe_unused]] ErrorCode error) {
                        return Result<std::string>::Success("Recovered");
                    });

                assert(recoveredResult.IsSuccess());
                assert(recoveredResult.GetValue() == "Recovered");
            }
        };

    }  // namespace Test
}  // namespace Engine

// Optional: Add a main function for standalone testing
#ifdef RUN_TESTS
int main() {
    Engine::Test::ErrorSystemTest::RunAllTests();
    return 0;
}
#endif
