#include "Renderer.hpp"
#include "compute_spv.h"
#include <iostream>
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace Engine::Graphics {

    Renderer::Renderer(Core::Window& window, Core::Config& config) 
        : m_window(window), m_config(config) {
        initVulkan();
        loadEmbeddedShaders();
        initImGui(); 
    }

    Renderer::~Renderer() {
        std::cout << "Cleaning up Vulkan and UI resources gracefully...\n";
        VkDevice device = m_vulkanContext->getDevice();
        vkDeviceWaitIdle(device);

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        if (m_imguiDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_imguiDescriptorPool, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, m_inFlightFences[i], nullptr);
        }

        if (m_commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, m_commandPool, nullptr);
        for (auto framebuffer : m_swapchainFramebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    void Renderer::initVulkan() {
        if (!glfwVulkanSupported()) throw std::runtime_error("Critical Error: Vulkan is not supported.");
        
        m_vulkanContext = std::make_unique<VulkanContext>(m_window);
        m_swapchain = std::make_unique<Swapchain>(*m_vulkanContext, m_window);
        m_renderPass = std::make_unique<RenderPass>(*m_vulkanContext, m_swapchain->getImageFormat());
        
        bool hwSupportsMesh = m_vulkanContext->getGpuSpecs().supportsMeshShaders;
        bool userWantsMesh = m_config.getBool("mesh_shaders", false);

        PipelineType typeToBuild = PipelineType::Traditional;
        if (hwSupportsMesh && userWantsMesh) {
            typeToBuild = PipelineType::Meshlet;
        }

        m_graphicsPipeline = std::make_unique<GraphicsPipeline>(*m_vulkanContext, m_renderPass->getHandle(), typeToBuild);

        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
        createImGuiDescriptorPool();
    }

    void Renderer::recreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window.getNativeWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window.getNativeWindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_vulkanContext->getDevice());

        for (auto framebuffer : m_swapchainFramebuffers) {
            vkDestroyFramebuffer(m_vulkanContext->getDevice(), framebuffer, nullptr);
        }
        m_swapchainFramebuffers.clear();

        m_swapchain = std::make_unique<Swapchain>(*m_vulkanContext, m_window);
        createFramebuffers();
        
        std::cout << "[Vulkan] Swapchain recreated successfully.\n";
    }

    void Renderer::createFramebuffers() {
        const auto& imageViews = m_swapchain->getImageViews();
        VkExtent2D extent = m_swapchain->getExtent();
        m_swapchainFramebuffers.resize(imageViews.size());

        for (size_t i = 0; i < imageViews.size(); i++) {
            VkImageView attachments[] = { imageViews[i] };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass->getHandle();
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_vulkanContext->getDevice(), &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Critical Error: Failed to create Vulkan Framebuffer!");
            }
        }
    }

    void Renderer::createCommandPool() {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanContext->getPhysicalDevice(), &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanContext->getPhysicalDevice(), &queueFamilyCount, queueFamilies.data());

        uint32_t graphicsFamilyIndex = 0;
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamilyIndex = i; break;
            }
        }

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; 
        poolInfo.queueFamilyIndex = graphicsFamilyIndex;

        if (vkCreateCommandPool(m_vulkanContext->getDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create Vulkan Command Pool!");
        }
    }

    void Renderer::createCommandBuffers() {
        m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

        if (vkAllocateCommandBuffers(m_vulkanContext->getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to allocate Vulkan Command Buffers!");
        }
    }

    void Renderer::createSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_vulkanContext->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_vulkanContext->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_vulkanContext->getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Critical Error: Failed to create synchronization objects!");
            }
        }
    }

    void Renderer::createImGuiDescriptorPool() {
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
        pool_info.pPoolSizes = pool_sizes;

        if (vkCreateDescriptorPool(m_vulkanContext->getDevice(), &pool_info, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create ImGui Descriptor Pool!");
        }
    }

    void Renderer::initImGui() {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui_ImplGlfw_InitForVulkan(m_window.getNativeWindow(), true);
        
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_vulkanContext->getInstance();
        init_info.PhysicalDevice = m_vulkanContext->getPhysicalDevice();
        init_info.Device = m_vulkanContext->getDevice();
        init_info.Queue = m_vulkanContext->getGraphicsQueue();
        init_info.DescriptorPool = m_imguiDescriptorPool;
        init_info.MinImageCount = 3; 
        init_info.ImageCount = static_cast<uint32_t>(m_swapchainFramebuffers.size());
        init_info.PipelineInfoMain.RenderPass = m_renderPass->getHandle();

        ImGui_ImplVulkan_Init(&init_info);
    }

    void Renderer::loadEmbeddedShaders() {
        std::cout << "Loaded embedded compute shader. Size: " << compute_spv_len << " bytes.\n";
    }

    void Renderer::uploadDataToSSBO(const std::vector<float>& data) {}

    void Renderer::beginUI() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Renderer::drawFrame(const glm::mat4& viewProj) {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        VkDevice device = m_vulkanContext->getDevice();

        vkWaitForFences(device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, m_swapchain->getHandle(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return; 
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Critical Error: Failed to acquire swapchain image!");
        }

        vkResetFences(device, 1, &m_inFlightFences[m_currentFrame]);

        VkCommandBuffer commandBuffer = m_commandBuffers[m_currentFrame];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass->getHandle();
        renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapchain->getExtent();

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->getHandle());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->getExtent().width);
        viewport.height = static_cast<float>(m_swapchain->getExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_swapchain->getExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // --- NEW: Push the Camera Matrix to the GPU ---
        vkCmdPushConstants(
            commandBuffer, 
            m_graphicsPipeline->getLayout(), 
            VK_SHADER_STAGE_VERTEX_BIT, 
            0, 
            sizeof(glm::mat4), 
            &viewProj
        );

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = {m_swapchain->getHandle()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(m_vulkanContext->getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasResized()) {
            m_window.resetResizedFlag();
            recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to present swapchain image!");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

} // namespace Engine::Graphics