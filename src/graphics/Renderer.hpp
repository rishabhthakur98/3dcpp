#pragma once

#include "../core/Window.hpp"
#include "../core/Config.hpp" // Needed to check user preferences
#include "VulkanContext.hpp"
#include "Swapchain.hpp"
#include "RenderPass.hpp"
#include "GraphicsPipeline.hpp"
#include <vector>
#include <memory>

namespace Engine::Graphics {

    class Renderer {
    public:
        // The Renderer now requires the Config object to read user settings
        Renderer(Core::Window& window, Core::Config& config);
        ~Renderer();

        void beginUI();
        void drawFrame();
        void uploadDataToSSBO(const std::vector<float>& data);

        const GpuSpecs& getGpuSpecs() const { return m_vulkanContext->getGpuSpecs(); }

    private:
        Core::Window& m_window;
        Core::Config& m_config;
        
        std::unique_ptr<VulkanContext> m_vulkanContext;
        std::unique_ptr<Swapchain> m_swapchain; 
        std::unique_ptr<RenderPass> m_renderPass;
        
        // --- NEW: The Graphics Pipeline Router ---
        std::unique_ptr<GraphicsPipeline> m_graphicsPipeline;
        
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

        void loadEmbeddedShaders();
        void initImGui();
    };

} // namespace Engine::Graphics