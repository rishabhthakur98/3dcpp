cmake_minimum_required(VERSION 3.20)
project(3dcpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/JoltPhysics/Build)

# --- 3. Shader Compilation & Embedding ---
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BIN_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

# Original compute shader
add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/compute_spv.h
    COMMAND ${CMAKE_SOURCE_DIR}/tools/dxc/bin/dxc -T cs_6_0 -E main ${SHADER_DIR}/compute.hlsl -spirv -Fo ${SHADER_BIN_DIR}/compute.spv
    COMMAND xxd -i compute.spv > compute_spv.h
    WORKING_DIRECTORY ${SHADER_BIN_DIR}
    DEPENDS ${SHADER_DIR}/compute.hlsl
)

# --- NEW: Compile Traditional Vertex Shader ---
add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/traditional_vert_spv.h
    COMMAND ${CMAKE_SOURCE_DIR}/tools/dxc/bin/dxc -T vs_6_0 -E main ${SHADER_DIR}/traditional_vert.hlsl -spirv -Fo ${SHADER_BIN_DIR}/traditional_vert.spv
    COMMAND xxd -i traditional_vert.spv > traditional_vert_spv.h
    WORKING_DIRECTORY ${SHADER_BIN_DIR}
    DEPENDS ${SHADER_DIR}/traditional_vert.hlsl
)

# --- NEW: Compile Traditional Fragment Shader ---
add_custom_command(
    OUTPUT ${SHADER_BIN_DIR}/traditional_frag_spv.h
    COMMAND ${CMAKE_SOURCE_DIR}/tools/dxc/bin/dxc -T ps_6_0 -E main ${SHADER_DIR}/traditional_frag.hlsl -spirv -Fo ${SHADER_BIN_DIR}/traditional_frag.spv
    COMMAND xxd -i traditional_frag.spv > traditional_frag_spv.h
    WORKING_DIRECTORY ${SHADER_BIN_DIR}
    DEPENDS ${SHADER_DIR}/traditional_frag.hlsl
)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB IMGUI_SRC 
    "vendor/imgui/*.cpp"
    "vendor/imgui/backends/imgui_impl_glfw.cpp"
    "vendor/imgui/backends/imgui_impl_vulkan.cpp"
)

add_executable(3dcpp_engine ${SRC_FILES} ${IMGUI_SRC})

# Force CMake to wait for all shaders to compile before building C++
add_custom_target(GenerateShaders DEPENDS 
    ${SHADER_BIN_DIR}/compute_spv.h
    ${SHADER_BIN_DIR}/traditional_vert_spv.h
    ${SHADER_BIN_DIR}/traditional_frag_spv.h
)
add_dependencies(3dcpp_engine GenerateShaders)

target_include_directories(3dcpp_engine PRIVATE 
    src vendor/glm vendor/vma/include vendor/JoltPhysics vendor/cgltf vendor/imgui vendor/imgui/backends ${SHADER_BIN_DIR}    
)

target_link_libraries(3dcpp_engine PRIVATE glfw Vulkan::Vulkan Jolt)