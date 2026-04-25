#include "VulkanBufferManager.hpp"
#include <stdexcept>
#include <iostream>
#include <cstring> 

namespace Engine::Graphics {

    VulkanBufferManager::VulkanBufferManager(VulkanContext& context, VkCommandPool commandPool)
        : m_context(context), m_commandPool(commandPool) {}

    VulkanBufferManager::~VulkanBufferManager() {
        freeUploadedModels();
        std::cout << "Vulkan Buffer Manager shut down safely.\n";
    }

    VkCommandBuffer VulkanBufferManager::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_context.getDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void VulkanBufferManager::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_context.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_context.getGraphicsQueue()); 

        vkFreeCommandBuffers(m_context.getDevice(), m_commandPool, 1, &commandBuffer);
    }

    void VulkanBufferManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;

        if (vmaCreateBuffer(m_context.getAllocator(), &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create VMA buffer!");
        }
    }

    void VulkanBufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        endSingleTimeCommands(commandBuffer);
    }

    void VulkanBufferManager::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("Critical Error: Unsupported Vulkan layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        endSingleTimeCommands(commandBuffer);
    }

    void VulkanBufferManager::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        endSingleTimeCommands(commandBuffer);
    }

    void VulkanBufferManager::uploadModel(std::shared_ptr<Model> model) {
        if (model->isUploaded) return;

        for (auto& mesh : model->meshes) {
            // VERTEXT BUFFER
            VkDeviceSize bufferSize = sizeof(Vertex) * mesh.vertices.size();
            VkBuffer stagingBuffer;
            VmaAllocation stagingAlloc;
            
            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAlloc);

            void* data;
            vmaMapMemory(m_context.getAllocator(), stagingAlloc, &data);
            memcpy(data, mesh.vertices.data(), (size_t)bufferSize);
            vmaUnmapMemory(m_context.getAllocator(), stagingAlloc);

            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mesh.vertexBuffer, mesh.vertexAllocation);
            copyBuffer(stagingBuffer, mesh.vertexBuffer, bufferSize);
            vmaDestroyBuffer(m_context.getAllocator(), stagingBuffer, stagingAlloc);

            // INDEX BUFFER
            if (!mesh.indices.empty()) {
                bufferSize = sizeof(uint32_t) * mesh.indices.size();
                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingAlloc);
                
                vmaMapMemory(m_context.getAllocator(), stagingAlloc, &data);
                memcpy(data, mesh.indices.data(), (size_t)bufferSize);
                vmaUnmapMemory(m_context.getAllocator(), stagingAlloc);

                createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mesh.indexBuffer, mesh.indexAllocation);
                copyBuffer(stagingBuffer, mesh.indexBuffer, bufferSize);
                vmaDestroyBuffer(m_context.getAllocator(), stagingBuffer, stagingAlloc);
            }
        }
        // Note: Actual texture upload and descriptor binding is handled in the Renderer 
        // to manage the global descriptor pools!
        model->isUploaded = true;
        m_uploadedModels.push_back(model); 
    }

    void VulkanBufferManager::freeUploadedModels() {
        for (auto& model : m_uploadedModels) {
            for (auto& mesh : model->meshes) {
                if (mesh.vertexBuffer) vmaDestroyBuffer(m_context.getAllocator(), mesh.vertexBuffer, mesh.vertexAllocation);
                if (mesh.indexBuffer) vmaDestroyBuffer(m_context.getAllocator(), mesh.indexBuffer, mesh.indexAllocation);
                mesh.vertexBuffer = VK_NULL_HANDLE;
                mesh.indexBuffer = VK_NULL_HANDLE;
                
                // Texture and Descriptor Sets are cleaned up automatically via RAII / Pool resets
                mesh.isUploaded = false;
            }
            model->isUploaded = false;
        }
        m_uploadedModels.clear();
        std::cout << "[Vulkan] Freed all physical VRAM buffers for models.\n";
    }

} // namespace Engine::Graphics