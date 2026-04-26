#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "VulkanContext.hpp"
#include "VulkanBufferManager.hpp"
#include <vector>
#include <array>
#include <cstdint>

namespace Engine::Graphics {

    class Texture {
    public:
        // Load from file
        Texture(VulkanContext& context, VulkanBufferManager& bufferManager, const std::vector<uint8_t>& rawData);
        // Fallback generator
        Texture(VulkanContext& context, VulkanBufferManager& bufferManager, const std::array<uint8_t, 4>& color);
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