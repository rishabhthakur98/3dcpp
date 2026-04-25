#include "RenderPass.hpp"
#include <stdexcept>
#include <iostream>

namespace Engine::Graphics {

    RenderPass::RenderPass(VulkanContext& context, VkFormat swapchainImageFormat)
        : m_context(context), m_renderPass(VK_NULL_HANDLE) {
        createRenderPass(swapchainImageFormat);
    }

    RenderPass::~RenderPass() {
        if (m_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_context.getDevice(), m_renderPass, nullptr);
        }
        std::cout << "Render Pass destroyed safely.\n";
    }

    void RenderPass::createRenderPass(VkFormat format) {
        // 1. Define the Color Attachment (Our Swapchain Image)
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling yet
        
        // Clear the screen to a solid color before drawing a new frame
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
        
        // Save the rendered pixels to memory so they can be shown on the screen
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
        
        // We aren't using stencil buffers right now
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        // The image starts in an undefined layout, but must transition to a 
        // PRESENT_SRC layout so the Linux window manager can display it
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // 2. Define the subpass (We only need one basic graphics subpass)
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // Index of the attachment array
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // 3. Define subpass dependencies to synchronize image transitions
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // The operation before our pass (the swapchain acquiring the image)
        dependency.dstSubpass = 0;                   // Our actual subpass
        
        // Wait until the image is completely ready before we start writing colors to it
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // 4. Create the Render Pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_context.getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create Vulkan Render Pass!");
        }

        std::cout << "Render Pass created successfully.\n";
    }

} // namespace Engine::Graphics