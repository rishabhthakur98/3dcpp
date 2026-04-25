#pragma once

#include <vulkan/vulkan.h>
#include "VulkanContext.hpp"
#include <string>
#include <vector>

namespace Engine::Graphics {

    enum class PipelineType { Traditional, Meshlet };

    class GraphicsPipeline {
    public:
        // Changed boolean face order to an intuitive int cullMode (0=Back, 1=Front)
        GraphicsPipeline(VulkanContext& context, VkRenderPass renderPass, PipelineType type, bool cullEnabled, int cullMode);
        ~GraphicsPipeline();

        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

        VkPipeline getHandle() const { return m_pipeline; }
        VkPipelineLayout getLayout() const { return m_pipelineLayout; }
        VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }
        
        PipelineType getType() const { return m_type; }

    private:
        VulkanContext& m_context;
        PipelineType m_type;
        
        bool m_cullEnabled;
        int m_cullMode;

        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout; 

        void createDescriptorSetLayout();
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);
        
        VkShaderModule createShaderModule(const unsigned char* code, size_t size);
    };

} // namespace Engine::Graphics