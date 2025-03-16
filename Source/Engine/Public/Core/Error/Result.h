
#pragma once

#include <optional>
#include <type_traits>

#include "Core/Error/ErrorCodes.h"
#include "Core/Error/ErrorSystem.h"

namespace Engine {

    // Forward declaration
    template <typename T>
    class Result;

    // Result class for operations that can fail
    template <typename T>
    class Result {
      public:
        // Constructor for success case
        static Result<T> Success(const T& value) { return Result<T>(value); }

        // Constructor for success case with move semantics
        static Result<T> Success(T&& value) {
            return Result<T>(std::move(value));
        }

        // Constructor for error case
        static Result<T> Failure(ErrorCode error) { return Result<T>(error); }

        // Check if the result is successful
        bool IsSuccess() const { return m_value.has_value(); }

        // Check if the result is a failure
        bool IsFailure() const { return !IsSuccess(); }

        // Get the error code (only valid if IsFailure() is true)
        ErrorCode GetError() const { return m_error; }

        // Get the error message (only valid if IsFailure() is true)
        std::string GetErrorMessage() const {
            return ErrorCodeManager::Get().GetErrorMessage(m_error);
        }

        // Get the value (only valid if IsSuccess() is true)
        const T& GetValue() const {
            if (!IsSuccess()) {
                REPORT_FATAL("Attempting to get value from failed result");
                // In a real application, you might want to throw an exception
                // here
                static T dummy{};
                return dummy;
            }
            return *m_value;
        }

        // Get the value (only valid if IsSuccess() is true)
        T& GetValue() {
            if (!IsSuccess()) {
                REPORT_FATAL("Attempting to get value from failed result");
                // In a real application, you might want to throw an exception
                // here
                static T dummy{};
                return dummy;
            }
            return *m_value;
        }

        // Chain operations with error propagation
        template <typename F>
        auto AndThen(F&& func) -> std::invoke_result_t<F, T> {
            if (IsSuccess()) {
                return func(GetValue());
            }
            return std::invoke_result_t<F, T>::Failure(GetError());
        }

        // Map successful value to another type
        template <typename F>
        auto Map(F&& func) -> Result<std::invoke_result_t<F, T>> {
            if (IsSuccess()) {
                return Result<std::invoke_result_t<F, T>>::Success(
                    func(GetValue()));
            }
            return Result<std::invoke_result_t<F, T>>::Failure(GetError());
        }

        // Handle error case
        template <typename F>
        Result<T> OrElse(F&& func) {
            if (IsFailure()) {
                return func(GetError());
            }
            return *this;
        }

        // Get value or default
        T ValueOr(const T& defaultValue) const {
            if (IsSuccess()) {
                return GetValue();
            }
            return defaultValue;
        }

      private:
        // Constructor for success case
        explicit Result(const T& value)
            : m_value(value), m_error(ERROR_SUCCESS) {}

        // Constructor for success case with move semantics
        explicit Result(T&& value)
            : m_value(std::move(value)), m_error(ERROR_SUCCESS) {}

        // Constructor for error case
        explicit Result(ErrorCode error)
            : m_value(std::nullopt), m_error(error) {}

        std::optional<T> m_value;
        ErrorCode m_error;
    };

    // Specialization for void
    template <>
    class Result<void> {
      public:
        static Result<void> Success() { return Result<void>(true); }

        static Result<void> Failure(ErrorCode error) {
            return Result<void>(error);
        }

        bool IsSuccess() const { return m_success; }
        bool IsFailure() const { return !IsSuccess(); }
        ErrorCode GetError() const { return m_error; }

        std::string GetErrorMessage() const {
            return ErrorCodeManager::Get().GetErrorMessage(m_error);
        }

        template <typename F>
        auto AndThen(F&& func) -> typename std::invoke_result_t<F> {
            if (IsSuccess()) {
                return func();
            }
            return decltype(func())::Failure(GetError());
        }

        template <typename F>
        Result<void> OrElse(F&& func) {
            if (IsFailure()) {
                return func(GetError());
            }
            return *this;
        }

      private:
        explicit Result(bool success)
            : m_success(success), m_error(ERROR_SUCCESS) {}
        explicit Result(ErrorCode error) : m_success(false), m_error(error) {}

        bool m_success;
        ErrorCode m_error;
    };

    // Helper function to create a successful Result
    template <typename T>
    Result<T> MakeSuccess(T&& value) {
        return Result<T>::Success(std::forward<T>(value));
    }

    // Helper function to create a failed Result
    template <typename T>
    Result<T> MakeFailure(ErrorCode error) {
        return Result<T>::Failure(error);
    }

    // Helper function to create a successful void Result
    inline Result<void> MakeSuccess() { return Result<void>::Success(); }

    // Helper function to create a failed void Result
    inline Result<void> MakeFailure(ErrorCode error) {
        return Result<void>::Failure(error);
    }

}  // namespace Engine
