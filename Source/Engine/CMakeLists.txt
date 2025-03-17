# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.20)

# Set project name
project(Engine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

set(CMAKE_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Add Core module
add_subdirectory(Core)

# Add Rendering module
add_subdirectory(Rendering)

# Create Engine library
add_library(${PROJECT_NAME} INTERFACE)

# Link with Core and Rendering modules
target_link_libraries(${PROJECT_NAME}
    INTERFACE
        Core
        Rendering
)




# Platform specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} INTERFACE PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} INTERFACE PLATFORM_LINUX)
endif()

# Set warning level
if(MSVC)
    target_compile_options(${PROJECT_NAME} INTERFACE /W4)
else()
    # target_compile_options(${PROJECT_NAME} INTERFACE  -Wextra -Wpedantic)
endif()
