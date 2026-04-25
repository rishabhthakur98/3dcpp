#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanContext.hpp"
#include "../core/Window.hpp"

namespace Engine::Graphics {

    // Manages the queue of images that are presented to the screen
    class Swapchain {
    public:
        Swapchain(VulkanContext& context, Core::Window& window);
        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        // Getters for the renderer
        VkSwapchainKHR getHandle() const { return m_swapchain; }
        VkFormat getImageFormat() const { return m_imageFormat; }
        VkExtent2D getExtent() const { return m_extent; }
        const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }

    private:
        VulkanContext& m_context;
        Core::Window& m_window;

        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_images;           // Raw memory buffers from the GPU
        std::vector<VkImageView> m_imageViews;   // The "lens" Vulkan uses to look at the images
        VkFormat m_imageFormat;
        VkExtent2D m_extent;

        // Internal setup functions
        void createSwapchain();
        void createImageViews();

        // Helpers to negotiate the best settings with the monitor
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    };

} // namespace Engine::Graphics