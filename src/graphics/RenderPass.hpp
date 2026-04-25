#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace Engine::Graphics {

    // Defines the blueprint for how rendering operations affect our Swapchain images
    class RenderPass {
    public:
        // --- THE FIX: Now officially accepts the Depth Buffer format ---
        RenderPass(VulkanContext& context, VkFormat swapchainImageFormat, VkFormat depthFormat);
        ~RenderPass();

        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;

        VkRenderPass getHandle() const { return m_renderPass; }

    private:
        VulkanContext& m_context;
        VkRenderPass m_renderPass;

        void createRenderPass(VkFormat colorFormat, VkFormat depthFormat);
    };

} // namespace Engine::Graphics