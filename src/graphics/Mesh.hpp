#pragma once
#include "Vertex.hpp"
#include <vector>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Engine::Graphics {

    struct Mesh {
        // CPU Data (RAM)
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        // GPU Handles (VRAM)
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexAllocation = VK_NULL_HANDLE;
        
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexAllocation = VK_NULL_HANDLE;

        bool isUploaded = false; // Tracks if the transfer to VRAM is complete
    };

} // namespace Engine::Graphics