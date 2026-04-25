#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h> 
#include <vector>
#include <string>
#include <optional>
#include "../core/Window.hpp"

namespace Engine::Graphics {

    struct GpuSpecs {
        std::string deviceName;
        std::string deviceType;
        std::string apiVersion;
        uint32_t dedicatedVRAM_MB;
        uint32_t maxTextureResolution;
        bool isDiscreteGpu;
        bool supportsHwRayTracing;
        bool supportsSwRayTracing;   
        bool supportsBindless;       
        bool supportsMeshShaders;    
        bool supportsVRS;            
        bool supportsMultiDrawIndirect; 
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    class VulkanContext {
    public:
        VulkanContext(Core::Window& window);
        ~VulkanContext();

        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;

        VkInstance getInstance() const { return m_instance; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        VkDevice getDevice() const { return m_device; }
        VkSurfaceKHR getSurface() const { return m_surface; } 
        VmaAllocator getAllocator() const { return m_allocator; }
        
        VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
        VkQueue getPresentQueue() const { return m_presentQueue; }

        // --- NEW: Expose the specs! ---
        const GpuSpecs& getGpuSpecs() const { return m_specs; }

    private:
        Core::Window& m_window;
        VkInstance m_instance;
        VkSurfaceKHR m_surface; 
        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;      
        VkQueue m_graphicsQueue; 
        VkQueue m_presentQueue;  
        VmaAllocator m_allocator; 
        GpuSpecs m_specs;

        void createInstance();
        void createSurface();
        void queryHardware();
        void createLogicalDevice();
        void initVMA();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device, const char* extensionName);
    };

} // namespace Engine::Graphics