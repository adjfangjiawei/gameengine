# Set project name
project(Core)

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
set(CORE_SOURCES
    Private/Assert.cpp
    Private/ConsoleUtils.cpp
    Private/Platform.cpp
    Private/Time.cpp
    Private/Log/LogSystem.cpp
    Private/Log/LogAnalytics.cpp
    Private/Log/LogConfig.cpp
    Private/Log/LogFilter.cpp
    Private/Log/LogPerformance.cpp
    Private/Log/LogSinks.cpp
    Private/Threading/ThreadPool.cpp
    Private/Memory/LinearAllocator.cpp
    Private/Memory/StackAllocator.cpp
    Private/Memory/HeapAllocator.cpp
    Private/Memory/PoolAllocator.cpp
    Private/Memory/MemoryDebugger.cpp
    Private/Memory/MemoryDefragmenter.cpp
    Private/Memory/MemoryProfiler.cpp
    Private/Memory/MemoryPoolManager.cpp
    Private/Memory/MemoryVisualizer.cpp
    Private/String/StringUtils.cpp
    Private/FileSystem/FileSystem.cpp
    Private/FileSystem/StandardFileSystem.cpp
    Private/FileSystem/FilePermissions.cpp
    Private/Config/ConfigSystem.cpp
    Private/Error/ErrorSystem.cpp
    Private/Error/ErrorCodes.cpp
    Private/Error/ErrorExample.cpp
)

# Add header files
set(CORE_HEADERS
    Public/Assert.h
    Public/ConsoleUtils.h
    Public/CoreNamespace.h
    Public/CoreTypes.h
    Public/Format.h
    Public/Input.h
    Public/Log/LogSystem.h
    Public/Platform.h
    Public/SignalHandler.h
    Public/Time.h
    Public/Window.h
    Public/Threading/ThreadPool.h
    Public/Threading/ThreadPoolConfig.h
    Public/Threading/ThreadPoolStats.h
    Public/Threading/ThreadTask.h
    Public/Memory/IAllocator.h
    Public/Memory/MemoryTypes.h
    Public/Memory/LinearAllocator.h
    Public/Memory/StackAllocator.h
    Public/Memory/HeapAllocator.h
    Public/Memory/PoolAllocator.h
    Public/Memory/MemoryDebugger.h
    Public/Memory/MemoryDefragmenter.h
    Public/Memory/MemoryProfiler.h
    Public/Memory/MemoryPoolManager.h
    Public/Memory/MemoryVisualizer.h
    Public/String/StringUtils.h
    Public/FileSystem/FileSystem.h
    Public/FileSystem/StandardFileSystem.h
    Public/FileSystem/FilePermissions.h
    Public/Error/ErrorSystem.h
    Public/Error/ErrorCodes.h
    Public/Error/Result.h
)

# Create library
add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Set include directories
target_include_directories(Core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${json_SOURCE_DIR}/include
        ${CMAKE_ENGINE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

option(BUILD_TESTS "Build the tests" ON)
if(BUILD_TESTS)
    # Add test executables
    set(TEST_FILES
        Private/Test/MemoryTest.cpp
        Private/Test/FileSystemTest.cpp
        Private/Test/ErrorSystemTest.cpp
        Private/Test/ConfigSystemTest.cpp
        Private/Test/StringUtilsTest.cpp
        Private/Test/MathTest.cpp
        Private/Test/CoreTest.cpp
        Private/Test/ReflectionTest.cpp
    )

    foreach(TEST_FILE ${TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE ${PROJECT_NAME})
        target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
        set_target_properties(${TEST_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()

# Find and link required packages
find_package(Threads REQUIRED)
find_package(ICU COMPONENTS uc i18n REQUIRED)
target_link_libraries(Core
    PRIVATE 
        Threads::Threads
        ICU::uc 
        ICU::i18n
)


# Platform specific settings
if(WIN32)
    target_compile_definitions(Core PRIVATE PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(Core PRIVATE PLATFORM_LINUX)
endif()

# Set warning level
if(MSVC)
    target_compile_options(Core PRIVATE /W4)
else()
    target_compile_options(Core PRIVATE -Wall -Wextra -Wpedantic)
endif()
