#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "VulkanContext.hpp"
#include "VulkanBufferManager.hpp"
#include <vector>
#include <cstdint>

namespace Engine::Graphics {

    class Texture {
    public:
        // Constructor for loading raw embedded PNG/JPEG data from the .glb file
        Texture(VulkanContext& context, VulkanBufferManager& bufferManager, const std::vector<uint8_t>& rawData);
        
        // Fallback constructor for generating a 1x1 solid white pixel if a model has no texture
        Texture(VulkanContext& context, VulkanBufferManager& bufferManager);
        
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        VkImageView getImageView() const { return m_imageView; }
        VkSampler getSampler() const { return m_sampler; }

    private:
        VulkanContext& m_context;
        
        VkImage m_image = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VkSampler m_sampler = VK_NULL_HANDLE;

        void createTextureImage(VulkanBufferManager& bufferManager, void* pixels, int width, int height);
        void createImageView();
        void createSampler();
    };

} // namespace Engine::Graphics