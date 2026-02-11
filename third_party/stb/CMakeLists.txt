cmake_minimum_required(VERSION 3.15)

project(
  
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
add_subdirectory(third_party/vk-bootstrap)
add_subdirectory(third_party/volk)
add_subdirectory(third_party/VulkanMemoryAllocator)
add_subdirectory(third_party/stb)

add_executable(bug_repro src/main.cpp)
target_link_libraries(bug_repro vk-bootstrap::vk-bootstrap Vulkan::Vulkan volk_headers GPUOpen::VulkanMemoryAllocator)

add_custom_command(
  TARGET bug_repro POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/src/*.spv
          ${CMAKE_CURRENT_BINARY_DIR}
)
