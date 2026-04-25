#pragma once

#include "../core/Window.hpp"
#include "../core/Config.hpp"
#include "VulkanContext.hpp"
#include "Swapchain.hpp"
#include "RenderPass.hpp"
#include "GraphicsPipeline.hpp"
#include "ModelLoader.hpp"
#include "../scene/SceneLoader.hpp"

#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace Engine::Graphics {

    class Renderer {
    public:
        Renderer(Core::Window& window, Core::Config& config);
        ~Renderer();

        void beginUI();
        
        // Now dynamically draws all entities passed to it!
        void drawFrame(const glm::mat4& viewProj, const std::vector<Scene::SceneEntity>& activeEntities, ModelLoader& modelLoader);
        
        // --- VRAM Management ---
        void uploadModel(std::shared_ptr<Model> model);
        void freeUploadedModels(); // Cleans the GPU to prevent memory leaks!
        
        void rebuildGraphicsPipeline();

        // Restored missing SSBO placeholder declaration
        void uploadDataToSSBO(const std::vector<float>& data);

        const GpuSpecs& getGpuSpecs() const { return m_vulkanContext->getGpuSpecs(); }

    private:
        Core::Window& m_window;
        Core::Config& m_config;
        
        std::unique_ptr<VulkanContext> m_vulkanContext;
        std::unique_ptr<Swapchain> m_swapchain; 
        std::unique_ptr<RenderPass> m_renderPass;
        std::unique_ptr<GraphicsPipeline> m_graphicsPipeline;
        
        std::vector<std::shared_ptr<Model>> m_uploadedModels;

        std::vector<VkFramebuffer> m_swapchainFramebuffers;
        VkCommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_commandBuffers;

        const int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t m_currentFrame = 0;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        VkDescriptorPool m_imguiDescriptorPool;

        void initVulkan();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void createImGuiDescriptorPool();
        void recreateSwapchain();

        // GPU Buffer Helpers
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        void initImGui();
        
        // Restored missing compute shader placeholder declaration
        void loadEmbeddedShaders();
    };

} // namespace Engine::Graphics