#pragma once

#include "VulkanContext.hpp"
#include "Model.hpp"
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Engine::Graphics {

    class VulkanBufferManager {
    public:
        VulkanBufferManager(VulkanContext& context, VkCommandPool commandPool);
        ~VulkanBufferManager();

        VulkanBufferManager(const VulkanBufferManager&) = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        void uploadModel(std::shared_ptr<Model> model);
        void freeUploadedModels();

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        
        // --- NEW: Image pipeline barriers and buffer copies ---
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    private:
        VulkanContext& m_context;
        VkCommandPool m_commandPool;
        std::vector<std::shared_ptr<Model>> m_uploadedModels;

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    };

} // namespace Engine::Graphics