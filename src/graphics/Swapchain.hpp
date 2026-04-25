#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.hpp"
#include "../core/Window.hpp"

namespace Engine::Graphics {

    class Swapchain {
    public:
        Swapchain(VulkanContext& context, Core::Window& window);
        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        VkSwapchainKHR getHandle() const { return m_swapchain; }
        VkFormat getImageFormat() const { return m_imageFormat; }
        VkExtent2D getExtent() const { return m_extent; }
        const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }

        // --- NEW: DEPTH BUFFER GETTERS ---
        VkFormat getDepthFormat() const { return m_depthFormat; }
        VkImageView getDepthImageView() const { return m_depthImageView; }

    private:
        VulkanContext& m_context;
        Core::Window& m_window;

        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_images;           
        std::vector<VkImageView> m_imageViews;   
        VkFormat m_imageFormat;
        VkExtent2D m_extent;

        // --- NEW: DEPTH BUFFER RESOURCES ---
        VkImage m_depthImage = VK_NULL_HANDLE;
        VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;
        VkImageView m_depthImageView = VK_NULL_HANDLE;
        VkFormat m_depthFormat;

        void createSwapchain();
        void createImageViews();
        
        // --- NEW: Depth Helpers ---
        void createDepthResources();
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    };

} // namespace Engine::Graphics