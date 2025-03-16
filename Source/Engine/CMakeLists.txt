# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.20)

# Set project name
project(Engine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Fetch and include nlohmann/json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Add source files
set(SOURCES
    Private/Core/Assert.cpp
    Private/Core/ConsoleUtils.cpp
    Private/Core/Platform.cpp
    Private/Core/Time.cpp
    Private/Core/Log/LogSystem.cpp
    Private/Core/Log/LogAnalytics.cpp
    Private/Core/Log/LogConfig.cpp
    Private/Core/Log/LogFilter.cpp
    Private/Core/Log/LogPerformance.cpp
    Private/Core/Log/LogSinks.cpp
    Private/Core/Threading/ThreadPool.cpp
    Private/Core/Memory/LinearAllocator.cpp
    Private/Core/Memory/StackAllocator.cpp
    Private/Core/Memory/HeapAllocator.cpp
    Private/Core/Memory/PoolAllocator.cpp
    Private/Core/Memory/MemoryDebugger.cpp
    Private/Core/Memory/MemoryDefragmenter.cpp
    Private/Core/Memory/MemoryProfiler.cpp
    Private/Core/Memory/MemoryPoolManager.cpp
    Private/Core/Memory/MemoryVisualizer.cpp
    "Private/Core/String/StringUtils.cpp"
    Private/Core/FileSystem/FileSystem.cpp
    Private/Core/FileSystem/StandardFileSystem.cpp
    Private/Core/FileSystem/FilePermissions.cpp
    Private/Core/Config/ConfigSystem.cpp
    # Add new error system files
    Private/Core/Error/ErrorSystem.cpp
    Private/Core/Error/ErrorCodes.cpp
    Private/Core/Error/ErrorExample.cpp
)

# Add header files
set(HEADERS
    Public/Core/Assert.h
    Public/Core/ConsoleUtils.h
    Public/Core/CoreNamespace.h
    Public/Core/CoreTypes.h
    Public/Core/Format.h
    Public/Core/Input.h
    Public/Core/Log/LogSystem.h
    Public/Core/Platform.h
    Public/Core/SignalHandler.h
    Public/Core/Time.h
    Public/Core/Window.h
    Public/Core/Threading/ThreadPool.h
    Public/Core/Threading/ThreadPoolConfig.h
    Public/Core/Threading/ThreadPoolStats.h
    Public/Core/Threading/ThreadTask.h
    Public/Core/Memory/IAllocator.h
    Public/Core/Memory/MemoryTypes.h
    Public/Core/Memory/LinearAllocator.h
    Public/Core/Memory/StackAllocator.h
    Public/Core/Memory/HeapAllocator.h
    Public/Core/Memory/PoolAllocator.h
    Public/Core/Memory/MemoryDebugger.h
    Public/Core/Memory/MemoryDefragmenter.h
    Public/Core/Memory/MemoryProfiler.h
    Public/Core/Memory/MemoryPoolManager.h
    Public/Core/Memory/MemoryVisualizer.h
    "Public/Core/String/StringUtils.h"
    Public/Core/FileSystem/FileSystem.h
    Public/Core/FileSystem/StandardFileSystem.h
    Public/Core/FileSystem/FilePermissions.h
    # Add new error system headers
    Public/Core/Error/ErrorSystem.h
    Public/Core/Error/ErrorCodes.h
    Public/Core/Error/Result.h
)

# Create library
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${json_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# Add compile definitions
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:BUILD_DEBUG>
        $<$<CONFIG:Release>:BUILD_RELEASE>
)

# Set output directories
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add tests if enabled
option(BUILD_TESTS "Build the tests" ON)
if(BUILD_TESTS)
    # Memory tests
    add_executable(MemoryTest Private/Core/Test/MemoryTest.cpp)
    target_link_libraries(MemoryTest PRIVATE ${PROJECT_NAME})
    target_include_directories(MemoryTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    set_target_properties(MemoryTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # FileSystem tests
    add_executable(FileSystemTest Private/Core/Test/FileSystemTest.cpp)
    target_link_libraries(FileSystemTest PRIVATE ${PROJECT_NAME})
    target_include_directories(FileSystemTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    set_target_properties(FileSystemTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Error System tests
    add_executable(ErrorSystemTest Private/Core/Test/ErrorSystemTest.cpp)
    target_link_libraries(ErrorSystemTest PRIVATE ${PROJECT_NAME})
    target_include_directories(ErrorSystemTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    target_compile_definitions(ErrorSystemTest PRIVATE RUN_TESTS)
    set_target_properties(ErrorSystemTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Config System tests
    add_executable(ConfigSystemTest Private/Core/Test/ConfigSystemTest.cpp)
    target_link_libraries(ConfigSystemTest PRIVATE ${PROJECT_NAME})
    target_include_directories(ConfigSystemTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    set_target_properties(ConfigSystemTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # String Utils tests
    add_executable(StringUtilsTest Private/Core/Test/StringUtilsTest.cpp)
    target_link_libraries(StringUtilsTest PRIVATE ${PROJECT_NAME})
    target_include_directories(StringUtilsTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    set_target_properties(StringUtilsTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Math tests
    add_executable(MathTest Private/Core/Test/MathTest.cpp)
    target_link_libraries(MathTest PRIVATE ${PROJECT_NAME})
    target_include_directories(MathTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    set_target_properties(MathTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Core tests
    add_executable(CoreTest Private/Core/Test/CoreTest.cpp)
    target_link_libraries(CoreTest PRIVATE ${PROJECT_NAME})
    target_include_directories(CoreTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    set_target_properties(CoreTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Reflection tests
    add_executable(ReflectionTest Private/Core/Test/ReflectionTest.cpp)
    target_link_libraries(ReflectionTest PRIVATE ${PROJECT_NAME})
    target_include_directories(ReflectionTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
    set_target_properties(ReflectionTest
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# Find and link required packages
find_package(Threads REQUIRED)
find_package(ICU COMPONENTS uc i18n REQUIRED)
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        Threads::Threads
        ICU::uc 
        ICU::i18n
)

# Platform specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE dbghelp)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()

# Set warning level
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Enable testing
enable_testing()

# Add tests to CTest
if(BUILD_TESTS)
    add_test(NAME MemoryTest COMMAND MemoryTest)
    add_test(NAME FileSystemTest COMMAND FileSystemTest)
    add_test(NAME ErrorSystemTest COMMAND ErrorSystemTest)
    add_test(NAME ConfigSystemTest COMMAND ConfigSystemTest)
    add_test(NAME StringUtilsTest COMMAND StringUtilsTest)
    add_test(NAME MathTest COMMAND MathTest)
    add_test(NAME CoreTest COMMAND CoreTest)
    add_test(NAME ReflectionTest COMMAND ReflectionTest)
endif()
