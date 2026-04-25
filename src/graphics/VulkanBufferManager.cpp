#include "VulkanBufferManager.hpp"
#include <stdexcept>
#include <iostream>
#include <cstring> // For memcpy

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
        vkQueueWaitIdle(m_context.getGraphicsQueue()); // Wait for physical transfer

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

    void VulkanBufferManager::uploadModel(std::shared_ptr<Model> model) {
        if (model->isUploaded) return;

        for (auto& mesh : model->meshes) {
            // 1. UPLOAD VERTICES
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

            // 2. UPLOAD INDICES
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
            mesh.isUploaded = true;
        }
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
                mesh.isUploaded = false;
            }
            model->isUploaded = false;
        }
        m_uploadedModels.clear();
        std::cout << "[Vulkan] Freed all physical VRAM buffers for models.\n";
    }

} // namespace Engine::Graphics