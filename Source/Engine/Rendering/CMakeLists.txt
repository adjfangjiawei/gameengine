# Set project name
project(Rendering)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)

#Find shaderc
pkg_check_modules(SHADERC REQUIRED shaderc)

# Find SPIRV-Cross components
find_library(SPIRV_CROSS_CORE_LIB spirv-cross-core)
find_library(SPIRV_CROSS_GLSL_LIB spirv-cross-glsl)
find_library(SPIRV_CROSS_HLSL_LIB spirv-cross-hlsl)
find_library(SPIRV_CROSS_CPP_LIB spirv-cross-cpp)

if(NOT SPIRV_CROSS_CORE_LIB OR NOT SPIRV_CROSS_GLSL_LIB)
    message(FATAL_ERROR "Required SPIRV-Cross libraries not found")
endif()



# Add source files
set(VULKAN_SOURCES
    Private/RHI/Example/RHIExample.cpp
    Private/RHI/Vulkan/VulkanAdvancedSync.cpp
    Private/RHI/Vulkan/VulkanAsyncCompute.cpp
    Private/RHI/Vulkan/VulkanCommandList.cpp
    Private/RHI/Vulkan/VulkanContext.cpp
    Private/RHI/Vulkan/VulkanDevice.cpp
    Private/RHI/Vulkan/VulkanEvent.cpp
    Private/RHI/Vulkan/VulkanFence.cpp
    Private/RHI/Vulkan/VulkanInstance.cpp
    Private/RHI/Vulkan/VulkanMemory.cpp
    Private/RHI/Vulkan/VulkanMeshShader.cpp
    Private/RHI/Vulkan/VulkanPhysicalDevice.cpp
    Private/RHI/Vulkan/VulkanPipeline.cpp
    Private/RHI/Vulkan/VulkanQueryHeap.cpp
    Private/RHI/Vulkan/VulkanRHI.cpp
    Private/RHI/Vulkan/VulkanRayTracing.cpp
    Private/RHI/Vulkan/VulkanResources.cpp
    Private/RHI/Vulkan/VulkanSamplerState.cpp
    Private/RHI/Vulkan/VulkanShaderCompiler.cpp
    Private/RHI/Vulkan/VulkanShaderFeedback.cpp
    Private/RHI/Vulkan/VulkanShaderHotReload.cpp
    Private/RHI/Vulkan/VulkanSparseResources.cpp
    Private/RHI/Vulkan/VulkanSwapChain.cpp
    Private/RHI/Vulkan/VulkanTimelineSemaphore.cpp
    Private/RHI/Vulkan/VulkanVariableRateShading.cpp
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
    Private/RHI/Vulkan/VulkanAdvancedSync.h
    Private/RHI/Vulkan/VulkanAsyncCompute.h
    Private/RHI/Vulkan/VulkanCommandList.h
    Private/RHI/Vulkan/VulkanCommandPoolManager.h
    Private/RHI/Vulkan/VulkanDevice.h
    Private/RHI/Vulkan/VulkanDevicePresent.h
    Private/RHI/Vulkan/VulkanEvent.h
    Private/RHI/Vulkan/VulkanFence.h
    Private/RHI/Vulkan/VulkanInstance.h
    Private/RHI/Vulkan/VulkanMemory.h
    Private/RHI/Vulkan/VulkanMemoryAllocator.h
    Private/RHI/Vulkan/VulkanMeshShader.h
    Private/RHI/Vulkan/VulkanPhysicalDevice.h
    Private/RHI/Vulkan/VulkanPipeline.h
    Private/RHI/Vulkan/VulkanQueryHeap.h
    Private/RHI/Vulkan/VulkanRHI.h
    Private/RHI/Vulkan/VulkanRayTracing.h
    Private/RHI/Vulkan/VulkanResources.h
    Private/RHI/Vulkan/VulkanSamplerState.h
    Private/RHI/Vulkan/VulkanShaderCompiler.h
    Private/RHI/Vulkan/VulkanShaderFeedback.h
    Private/RHI/Vulkan/VulkanShaderHotReload.h
    Private/RHI/Vulkan/VulkanSparseResources.h
    Private/RHI/Vulkan/VulkanSwapChain.h
    Private/RHI/Vulkan/VulkanTimelineSemaphore.h
    Private/RHI/Vulkan/VulkanTypeOperators.h
    Private/RHI/Vulkan/VulkanVariableRateShading.h
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
        /usr/include/spirv_cross
        ${SHADERC_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# Link libraries
target_link_libraries(Rendering 
    PRIVATE 
        Vulkan::Vulkan
        ${SPIRV_CROSS_CORE_LIB}
        ${SPIRV_CROSS_GLSL_LIB}
        ${SPIRV_CROSS_HLSL_LIB}
        ${SPIRV_CROSS_CPP_LIB}
        ${SHADERC_LIBRARIES}
)

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
    # target_compile_options(Rendering PRIVATE -Wall -Wextra -Wpedantic)
endif()
