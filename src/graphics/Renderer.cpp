#include "Renderer.hpp"
#include <iostream>
#include <stdexcept>
#include <array>
#include <cstring>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine::Graphics {

    Renderer::Renderer(Core::Window& window, Core::Config& config) 
        : m_window(window), m_config(config) {
        initVulkan();
        initImGui(); 
    }

    Renderer::~Renderer() {
        std::cout << "Cleaning up Vulkan and UI resources gracefully...\n";
        VkDevice device = m_vulkanContext->getDevice();
        vkDeviceWaitIdle(device);

        freeUploadedModels();
        m_defaultTexture.reset();
        
        for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vmaUnmapMemory(m_vulkanContext->getAllocator(), m_globalSsboAllocations[i]);
            vmaDestroyBuffer(m_vulkanContext->getAllocator(), m_globalSsboBuffers[i], m_globalSsboAllocations[i]);
        }
        
        m_bufferManager.reset();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        if (m_imguiDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_imguiDescriptorPool, nullptr);
        if (m_descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        
        if (m_globalDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_globalDescriptorSetLayout, nullptr);
        if (m_materialDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_materialDescriptorSetLayout, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, m_inFlightFences[i], nullptr);
        }

        for (auto framebuffer : m_swapchainFramebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
        if (m_commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, m_commandPool, nullptr);
    }

    void Renderer::initVulkan() {
        if (!glfwVulkanSupported()) throw std::runtime_error("Critical Error: Vulkan is not supported.");
        
        m_vulkanContext = std::make_unique<VulkanContext>(m_window);
        m_swapchain = std::make_unique<Swapchain>(*m_vulkanContext, m_window);
        m_renderPass = std::make_unique<RenderPass>(*m_vulkanContext, m_swapchain->getImageFormat(), m_swapchain->getDepthFormat());
        
        createCommandPool();
        m_bufferManager = std::make_unique<VulkanBufferManager>(*m_vulkanContext, m_commandPool);

        // =========================================================================
        // 1. CREATE SET 0: GLOBAL SSBO 
        // =========================================================================
        VkDescriptorSetLayoutBinding ssboBinding{};
        ssboBinding.binding = 0;
        ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssboBinding.descriptorCount = 1;
        ssboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        
        VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
        globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        globalLayoutInfo.bindingCount = 1;
        globalLayoutInfo.pBindings = &ssboBinding;
        vkCreateDescriptorSetLayout(m_vulkanContext->getDevice(), &globalLayoutInfo, nullptr, &m_globalDescriptorSetLayout);

        // =========================================================================
        // 2. CREATE SET 1: MATERIAL TEXTURES
        // =========================================================================
        std::vector<VkDescriptorSetLayoutBinding> bindings(4);
        for(int i = 0; i < 4; i++) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount = 1;
            
            // --- THE DISPLACEMENT FIX ---
            // Allow the Vertex Shader to read from the bound Height map!
            bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; 
            
            bindings[i].pImmutableSamplers = nullptr;
        }
        
        VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
        materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        materialLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        materialLayoutInfo.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(m_vulkanContext->getDevice(), &materialLayoutInfo, nullptr, &m_materialDescriptorSetLayout);

        // =========================================================================
        // 3. DESCRIPTOR POOL
        // =========================================================================
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 4000; 
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; 
        poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT; 
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1000;
        vkCreateDescriptorPool(m_vulkanContext->getDevice(), &poolInfo, nullptr, &m_descriptorPool);

        // =========================================================================
        // 4. ALLOCATE AND MAP SSBO BUFFERS
        // =========================================================================
        m_globalSsboBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_globalSsboAllocations.resize(MAX_FRAMES_IN_FLIGHT);
        m_globalSsboMapped.resize(MAX_FRAMES_IN_FLIGHT);
        m_globalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorSetLayout> globalLayouts(MAX_FRAMES_IN_FLIGHT, m_globalDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = globalLayouts.data();
        vkAllocateDescriptorSets(m_vulkanContext->getDevice(), &allocInfo, m_globalDescriptorSets.data());

        for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_bufferManager->createBuffer(sizeof(GlobalSSBO), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, m_globalSsboBuffers[i], m_globalSsboAllocations[i]);
            vmaMapMemory(m_vulkanContext->getAllocator(), m_globalSsboAllocations[i], &m_globalSsboMapped[i]);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_globalSsboBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(GlobalSSBO);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_globalDescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; 
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_vulkanContext->getDevice(), 1, &descriptorWrite, 0, nullptr);
        }

        std::array<uint8_t, 4> white = {255, 255, 255, 255};
        m_defaultTexture = std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, white);

        rebuildGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
        createSyncObjects();
        createImGuiDescriptorPool();
    }

    void Renderer::rebuildGraphicsPipeline() {
        if (m_vulkanContext->getDevice() != VK_NULL_HANDLE) vkDeviceWaitIdle(m_vulkanContext->getDevice());

        bool cullEnabled = m_config.getBool("cull_enabled", false);
        int cullMode = m_config.getInt("cull_mode", 0);

        std::vector<VkDescriptorSetLayout> layouts = { m_globalDescriptorSetLayout, m_materialDescriptorSetLayout };

        m_graphicsPipeline.reset();
        m_graphicsPipeline = std::make_unique<GraphicsPipeline>(*m_vulkanContext, m_renderPass->getHandle(), PipelineType::Traditional, cullEnabled, cullMode, layouts);
    }

    void Renderer::recreateSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window.getNativeWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window.getNativeWindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_vulkanContext->getDevice());

        for (auto framebuffer : m_swapchainFramebuffers) vkDestroyFramebuffer(m_vulkanContext->getDevice(), framebuffer, nullptr);
        m_swapchainFramebuffers.clear();

        m_swapchain = std::make_unique<Swapchain>(*m_vulkanContext, m_window);
        createFramebuffers();
    }

    void Renderer::createFramebuffers() {
        const auto& imageViews = m_swapchain->getImageViews();
        VkExtent2D extent = m_swapchain->getExtent();
        m_swapchainFramebuffers.resize(imageViews.size());

        for (size_t i = 0; i < imageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = { imageViews[i], m_swapchain->getDepthImageView() };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass->getHandle();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;
            vkCreateFramebuffer(m_vulkanContext->getDevice(), &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]);
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
        vkCreateCommandPool(m_vulkanContext->getDevice(), &poolInfo, nullptr, &m_commandPool);
    }

    void Renderer::createCommandBuffers() {
        m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
        vkAllocateCommandBuffers(m_vulkanContext->getDevice(), &allocInfo, m_commandBuffers.data());
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
            vkCreateSemaphore(m_vulkanContext->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
            vkCreateSemaphore(m_vulkanContext->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
            vkCreateFence(m_vulkanContext->getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]);
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
        vkCreateDescriptorPool(m_vulkanContext->getDevice(), &pool_info, nullptr, &m_imguiDescriptorPool);
    }

    void Renderer::initImGui() {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.IniFilename = nullptr; 
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

    void Renderer::uploadModel(std::shared_ptr<Model> model) {
        if (model->isUploaded) return;

        m_bufferManager->uploadModel(model);

        for (auto& mesh : model->meshes) {
            std::array<uint8_t, 4> white = {255, 255, 255, 255};
            std::array<uint8_t, 4> flatNormal = {128, 128, 255, 255};
            std::array<uint8_t, 4> defaultMetRough = {0, 255, 0, 255}; 
            std::array<uint8_t, 4> defaultHeight = {255, 255, 255, 255};

            mesh.albedoTex = mesh.rawAlbedoData.empty() ? std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, white) 
                                                        : std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, mesh.rawAlbedoData);
            mesh.normalTex = mesh.rawNormalData.empty() ? std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, flatNormal) 
                                                        : std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, mesh.rawNormalData);
            mesh.metRoughTex = mesh.rawMetRoughData.empty() ? std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, defaultMetRough) 
                                                            : std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, mesh.rawMetRoughData);
            mesh.heightTex = mesh.rawHeightData.empty() ? std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, defaultHeight) 
                                                        : std::make_shared<Texture>(*m_vulkanContext, *m_bufferManager, mesh.rawHeightData);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_materialDescriptorSetLayout;
            vkAllocateDescriptorSets(m_vulkanContext->getDevice(), &allocInfo, &mesh.descriptorSet);

            std::array<VkDescriptorImageInfo, 4> imageInfos{};
            imageInfos[0] = {mesh.albedoTex->getSampler(), mesh.albedoTex->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            imageInfos[1] = {mesh.normalTex->getSampler(), mesh.normalTex->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            imageInfos[2] = {mesh.metRoughTex->getSampler(), mesh.metRoughTex->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            imageInfos[3] = {mesh.heightTex->getSampler(), mesh.heightTex->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

            std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
            for(int i=0; i<4; i++) {
                descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[i].dstSet = mesh.descriptorSet;
                descriptorWrites[i].dstBinding = i;
                descriptorWrites[i].dstArrayElement = 0;
                descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[i].descriptorCount = 1;
                descriptorWrites[i].pImageInfo = &imageInfos[i];
            }

            vkUpdateDescriptorSets(m_vulkanContext->getDevice(), 4, descriptorWrites.data(), 0, nullptr);
            mesh.isUploaded = true;
        }
        model->isUploaded = true;
    }

    void Renderer::freeUploadedModels() {
        vkDeviceWaitIdle(m_vulkanContext->getDevice());
        m_bufferManager->freeUploadedModels();
    }

    void Renderer::beginUI() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Renderer::drawFrame(const glm::mat4& viewProj, const glm::vec3& camPos, const std::vector<Scene::SceneEntity>& activeEntities, ModelLoader& modelLoader) {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        VkDevice device = m_vulkanContext->getDevice();

        vkWaitForFences(device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, m_swapchain->getHandle(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapchain(); return; } 
        
        vkResetFences(device, 1, &m_inFlightFences[m_currentFrame]);

        VkCommandBuffer commandBuffer = m_commandBuffers[m_currentFrame];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // =========================================================================
        // --- SSBO UPDATE ---
        // =========================================================================
        GlobalSSBO ssbo{};
        ssbo.viewProj = viewProj;
        ssbo.camPos = glm::vec4(camPos, 1.0f);
        ssbo.pomScale = m_config.getFloat("pom_scale", 0.05f);
        ssbo.usePOM = m_config.getBool("enable_pom", true) ? 1 : 0;
        ssbo.usePBR = m_config.getBool("enable_pbr", true) ? 1 : 0;
        
        // Push the new UI displacement settings securely across the bus
        ssbo.useDisplacement = m_config.getBool("enable_displacement", true) ? 1 : 0;
        ssbo.displacementScale = m_config.getFloat("displacement_scale", 0.1f);
        
        memcpy(m_globalSsboMapped[m_currentFrame], &ssbo, sizeof(GlobalSSBO));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass->getHandle();
        renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapchain->getExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color.float32[0] = 0.0f;
        clearValues[0].color.float32[1] = 0.0f;
        clearValues[0].color.float32[2] = 0.0f;
        clearValues[0].color.float32[3] = 1.0f;
        clearValues[1].depthStencil.depth = 1.0f;
        clearValues[1].depthStencil.stencil = 0;

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->getHandle());

        VkViewport viewport{};
        viewport.x = 0.0f; viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->getExtent().width);
        viewport.height = static_cast<float>(m_swapchain->getExtent().height);
        viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0}; scissor.extent = m_swapchain->getExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->getLayout(), 0, 1, &m_globalDescriptorSets[m_currentFrame], 0, nullptr);

        for (const auto& entity : activeEntities) {
            if (!entity.props.visible) continue;

            auto model = modelLoader.loadModel(entity.modelPath);
            if (!model || !model->isUploaded) continue;

            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, entity.transform.position);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.transform.rotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.transform.rotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.transform.rotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::scale(modelMatrix, entity.transform.scale);

            vkCmdPushConstants(commandBuffer, m_graphicsPipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &modelMatrix);

            for (const auto& mesh : model->meshes) {
                if (!mesh.isUploaded) continue;

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->getLayout(), 1, 1, &mesh.descriptorSet, 0, nullptr);

                VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                
                if (mesh.indexBuffer != VK_NULL_HANDLE) {
                    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
                } else {
                    vkCmdDraw(commandBuffer, static_cast<uint32_t>(mesh.vertices.size()), 1, 0, 0);
                }
            }
        }

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

        vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]);

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
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

} // namespace Engine::Graphics