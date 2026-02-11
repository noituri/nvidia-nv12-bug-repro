cmake_minimum_required(VERSION 3.20)

project(
  nvidia-nv12-bug-repro
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
add_subdirectory(third_party/vk-bootstrap)
add_subdirectory(third_party/volk)
add_subdirectory(third_party/VulkanMemoryAllocator)
include_directories(third_party/stb)

add_executable(bug_repro
  src/main.cpp
  src/utils.h
  src/utils.cpp
  src/gpu_context.h
  src/gpu_context.cpp
)
target_link_libraries(bug_repro
  vk-bootstrap::vk-bootstrap
  Vulkan::Vulkan volk_headers
  GPUOpen::VulkanMemoryAllocator
)

file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/src/shaders/*")
set(SHADER_COMMANDS)

foreach(SHADER_SOURCE ${SHADER_FILES})
  cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
  cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)

  message(${SHADER_SOURCE})

  list(APPEND SHADER_COMMANDS COMMAND)
  list(APPEND SHADER_COMMANDS glslc)
  list(APPEND SHADER_COMMANDS "${SHADER_SOURCE}")
  list(APPEND SHADER_COMMANDS "-o")
  list(APPEND SHADER_COMMANDS "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")
endforeach()


add_custom_target(
  run ALL
  ${SHADER_COMMANDS}
)
