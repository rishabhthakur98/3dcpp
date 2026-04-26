#include "Texture.hpp"
#include "../../vendor/stb_image.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace Engine::Graphics {

    Texture::Texture(VulkanContext& context, VulkanBufferManager& bufferManager, const std::vector<uint8_t>& rawData) 
        : m_context(context) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load_from_memory(rawData.data(), static_cast<int>(rawData.size()), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) throw std::runtime_error("Critical Error: Failed to decode texture image from GLB memory!");
        createTextureImage(bufferManager, pixels, texWidth, texHeight);
        stbi_image_free(pixels); 
        createImageView();
        createSampler();
    }

    Texture::Texture(VulkanContext& context, VulkanBufferManager& bufferManager, const std::array<uint8_t, 4>& color) 
        : m_context(context) {
        // Generate a 1x1 fallback pixel using the requested color
        createTextureImage(bufferManager, (void*)color.data(), 1, 1);
        createImageView();
        createSampler();
    }

    Texture::~Texture() {
        if (m_sampler != VK_NULL_HANDLE) vkDestroySampler(m_context.getDevice(), m_sampler, nullptr);
        if (m_imageView != VK_NULL_HANDLE) vkDestroyImageView(m_context.getDevice(), m_imageView, nullptr);
        if (m_image != VK_NULL_HANDLE) vmaDestroyImage(m_context.getAllocator(), m_image, m_allocation);
    }

    void Texture::createTextureImage(VulkanBufferManager& bufferManager, void* pixels, int width, int height) {
        VkDeviceSize imageSize = width * height * 4; 
        VkBuffer stagingBuffer; VmaAllocation stagingAlloc;
        
        bufferManager.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAlloc);

        void* data;
        vmaMapMemory(m_context.getAllocator(), stagingAlloc, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vmaUnmapMemory(m_context.getAllocator(), stagingAlloc);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        // PBR requires unORM for data maps (Normal/Roughness), but SRGB for Albedo. 
        // We use UNORM generally here and let the shader parse it safely to prevent artifacting.
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; 
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateImage(m_context.getAllocator(), &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create Vulkan Texture Image!");
        }

        bufferManager.transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        bufferManager.copyBufferToImage(stagingBuffer, m_image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        bufferManager.transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vmaDestroyBuffer(m_context.getAllocator(), stagingBuffer, stagingAlloc);
    }

    void Texture::createImageView() {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(m_context.getDevice(), &viewInfo, nullptr, &m_imageView);
    }

    void Texture::createSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR; 
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; 
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_context.getPhysicalDevice(), &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        vkCreateSampler(m_context.getDevice(), &samplerInfo, nullptr, &m_sampler);
    }

} // namespace Engine::Graphics