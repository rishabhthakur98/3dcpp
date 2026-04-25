#pragma once
#include "Vertex.hpp"
#include <vector>
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Engine::Graphics {

    class Texture; // Forward declaration

    struct Mesh {
        // CPU Data
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        // Raw embedded PNG/JPEG data extracted directly from the .glb file
        std::vector<uint8_t> rawTextureData;

        // GPU Resources
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexAllocation = VK_NULL_HANDLE;
        
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexAllocation = VK_NULL_HANDLE;
        
        std::shared_ptr<Texture> texture = nullptr;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        bool isUploaded = false;
    };

} // namespace Engine::Graphics