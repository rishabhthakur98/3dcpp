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
        // Requires the context and the Command Pool to issue transfer commands
        VulkanBufferManager(VulkanContext& context, VkCommandPool commandPool);
        ~VulkanBufferManager();

        VulkanBufferManager(const VulkanBufferManager&) = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        // The public API for transferring CPU RAM models into GPU VRAM
        void uploadModel(std::shared_ptr<Model> model);
        void freeUploadedModels();

        // Generic buffer creation helpers
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    private:
        VulkanContext& m_context;
        VkCommandPool m_commandPool;
        std::vector<std::shared_ptr<Model>> m_uploadedModels;

        // Command buffer helpers for memory transfers
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    };

} // namespace Engine::Graphics