cmake_minimum_required(VERSION 3.20)
project(3dcpp VERSION 1.0 LANGUAGES CXX)

# Enforce C++20 for modern language features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- 1. Find System Vulkan ---
# Link to the system's Vulkan loader (libvulkan.so) to communicate with the GPU
find_package(Vulkan REQUIRED)

# --- 2. Add Vendored Libraries (Window & Input) ---
# Build GLFW locally from the vendor folder.
# We disable tests and examples to keep the compile time fast and strictly local.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

# --- 3. Add Vendored Libraries (Physics) ---
# Build Jolt Physics locally. 
# We explicitly turn off Jolt's internal GPU compute features (Vulkan/DX12/Metal)
# so it does not look for the DXC compiler and fail the build.
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)

# Disable Jolt's GPU features to bypass shader compilation
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/JoltPhysics/Build)

# --- 4. Engine Core ---
# Gather all segregated C++ source files from the src directory and its subfolders
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# Create the main engine executable
add_executable(3dcpp_engine ${SRC_FILES})

# --- 5. Include Directories ---
# Define where the compiler should look for header files, including header-only libs
target_include_directories(3dcpp_engine PRIVATE 
    src
    vendor/glm
    vendor/vma/include
    vendor/JoltPhysics
)

# --- 6. Link Libraries ---
# Link our executable with the local vendor libraries and the system Vulkan loader
target_link_libraries(3dcpp_engine PRIVATE 
    glfw 
    Vulkan::Vulkan
    Jolt 
)