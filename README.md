# 3D C++ Engine (Vulkan + HLSL)

A cutting-edge, bindless Vulkan game engine built from scratch in C++20 on Linux Mint.

## Features
- **Bindless Architecture:** Exclusively utilizes SSBOs instead of UBOs for GPU-driven rendering.
- **Embedded Shaders:** HLSL shaders are compiled via DXC and baked directly into the C++ binary.
- **Native Fullscreen:** Automatically detects primary monitor resolution for seamless fullscreen.
- **Self-Contained:** Zero system dependency hassle. All libraries are vendored.

## Dependencies (Vendored)
- GLFW (Window & Input)
- Jolt Physics (CPU Rigid Body Physics)
- GLM (Mathematics)
- VMA (Vulkan Memory Management)
- cgltf (GLTF / GLB model loading)
- DXC (DirectX Shader Compiler - Local in `tools/dxc/`)

## How to Compile

1. Ensure you have standard Linux build tools and Vulkan headers:
   `sudo apt install build-essential cmake xxd libvulkan-dev vulkan-headers`
2. Create and enter the build directory:
   `mkdir build && cd build`
3. Configure and compile:
   `cmake ..`
   `make -j$(nproc)`
4. Run the engine:
   `./3dcpp_engine`