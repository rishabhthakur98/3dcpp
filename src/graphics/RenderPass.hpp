#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"

namespace Engine::Graphics {

    // Defines the blueprint for how rendering operations affect our Swapchain images
    class RenderPass {
    public:
        // Requires the context (to get the device) and the format of the swapchain images
        RenderPass(VulkanContext& context, VkFormat swapchainImageFormat);
        ~RenderPass();

        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;

        VkRenderPass getHandle() const { return m_renderPass; }

    private:
        VulkanContext& m_context;
        VkRenderPass m_renderPass;

        void createRenderPass(VkFormat format);
    };

} // namespace Engine::Graphics