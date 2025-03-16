
# Set minimum CMake version
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "clang++-21")
set(CMAKE_C_COMPILER "clang-21")
# Set project name and version
project(GameEngine VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add compile definitions based on platform
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS=1 -DPLATFORM_LINUX=0 -DPLATFORM_MAC=0)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_WINDOWS=0 -DPLATFORM_LINUX=1 -DPLATFORM_MAC=0)
elseif(APPLE)
    add_definitions(-DPLATFORM_WINDOWS=0 -DPLATFORM_LINUX=0 -DPLATFORM_MAC=1)
endif()

# Add compile definitions based on configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DBUILD_DEBUG=1)
else()
    add_definitions(-DBUILD_DEBUG=0)
endif()

# Set compiler flags
if(MSVC)
    # Windows/MSVC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    # GCC/Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-deprecated-literal-operator")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Option for building tests
option(BUILD_TESTS "Build the test programs" ON)

# Add subdirectories
add_subdirectory(Source/Engine)

# Testing setup
if(BUILD_TESTS)
    enable_testing()
    
    # Add test targets
    # add_test(NAME CoreTest 
    #          COMMAND $<TARGET_FILE:CoreTest>
    #          WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
             
    # add_test(NAME FileSystemTest 
    #          COMMAND $<TARGET_FILE:FileSystemTest>
    #          WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
             
    # add_test(NAME FilePermissionsTest 
    #          COMMAND $<TARGET_FILE:FilePermissionsTest>
    #          WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

# Install rules
install(DIRECTORY Source/Engine/Public/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

# Package configuration
include(CPack)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Game Engine Core Library")
set(CPACK_PACKAGE_VENDOR "Game Engine Team")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

# Custom targets
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_clean.cmake"
)

# Documentation target (if Doxygen is found)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Project configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
