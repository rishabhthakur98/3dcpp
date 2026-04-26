#pragma once
#include "Vertex.hpp"
#include <vector>
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Engine::Graphics {

    class Texture; 

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        
        // PBR Raw Data Blocks
        std::vector<uint8_t> rawAlbedoData;
        std::vector<uint8_t> rawNormalData;
        std::vector<uint8_t> rawMetRoughData;
        std::vector<uint8_t> rawHeightData;

        // VRAM Buffers
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexAllocation = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexAllocation = VK_NULL_HANDLE;
        
        // AAA Texture Set
        std::shared_ptr<Texture> albedoTex = nullptr;
        std::shared_ptr<Texture> normalTex = nullptr;
        std::shared_ptr<Texture> metRoughTex = nullptr;
        std::shared_ptr<Texture> heightTex = nullptr;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        bool isUploaded = false;
    };

} // namespace Engine::Graphics