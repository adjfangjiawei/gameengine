# Set project name
project(Rendering)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Add source files
set(VULKAN_SOURCES
    Private/RHI/Example/RHIExample.cpp
    Private/RHI/Vulkan/VulkanCommandList.cpp
    Private/RHI/Vulkan/VulkanContext.cpp
    Private/RHI/Vulkan/VulkanPipeline.cpp
    Private/RHI/Vulkan/VulkanRHI.cpp
    Private/RHI/Vulkan/VulkanResources.cpp
    Private/RHI/Vulkan/VulkanSwapChain.cpp
)

# Add header files
set(VULKAN_HEADERS
    Public/RHI/RHI.h
    Public/RHI/RHICommandList.h
    Public/RHI/RHIContext.h
    Public/RHI/RHIDefinitions.h
    Public/RHI/RHIDevice.h
    Public/RHI/RHIModule.h
    Public/RHI/RHIOperators.h
    Public/RHI/RHIResources.h
    Private/RHI/Vulkan/VulkanCommandList.h
    Private/RHI/Vulkan/VulkanDevicePresent.h
    Private/RHI/Vulkan/VulkanPipeline.h
    Private/RHI/Vulkan/VulkanRHI.h
    Private/RHI/Vulkan/VulkanResources.h
    Private/RHI/Vulkan/VulkanSwapChain.h
    Private/RHI/Vulkan/VulkanTypeOperators.h
)

# Create library
add_library(Rendering STATIC ${VULKAN_SOURCES} ${VULKAN_HEADERS})

# Set include directories
target_include_directories(Rendering
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_ENGINE_DIR}
        ${CMAKE_ENGINE_DIR}/Core/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 链接vulkan
target_link_libraries(Rendering PRIVATE Vulkan::Vulkan)

# # Add tests if enabled
# option(BUILD_TESTS "Build the tests" ON)
# if(BUILD_TESTS)
#     # Add test executables
#     set(TEST_FILES
#         Private/Test/RHITest.cpp
#         Private/Test/VulkanTest.cpp
#     )

#     foreach(TEST_FILE ${TEST_FILES})
#         get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
#         add_executable(${TEST_NAME} ${TEST_FILE})
#         target_link_libraries(${TEST_NAME} PRIVATE ${PROJECT_NAME})
#         target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Public)
#         set_target_properties(${TEST_NAME}
#             PROPERTIES
#             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
#         )
#         add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#     endforeach()
# endif()


# Platform specific settings
if(WIN32)
    target_compile_definitions(Rendering  PRIVATE PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(Rendering PRIVATE PLATFORM_LINUX)
endif()

# Set warning level
if(MSVC)
    target_compile_options(Rendering PRIVATE /W4)
else()
    target_compile_options(Rendering PRIVATE -Wall -Wextra -Wpedantic)
endif()
