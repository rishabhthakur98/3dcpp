#include "Swapchain.hpp"
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <limits> // <-- THE FIX: Required for std::numeric_limits

namespace Engine::Graphics {

    Swapchain::Swapchain(VulkanContext& context, Core::Window& window)
        : m_context(context), m_window(window), m_swapchain(VK_NULL_HANDLE) {
        
        // Initialize the presentation queue and the image views for the screen
        createSwapchain();
        createImageViews();
    }

    Swapchain::~Swapchain() {
        // Safely destroy all image views (the lenses looking at the swapchain images)
        for (auto imageView : m_imageViews) {
            vkDestroyImageView(m_context.getDevice(), imageView, nullptr);
        }
        
        // Destroy the core swapchain infrastructure
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_context.getDevice(), m_swapchain, nullptr);
        }
        
        std::cout << "Swapchain destroyed safely.\n";
    }

    void Swapchain::createSwapchain() {
        VkPhysicalDevice physicalDevice = m_context.getPhysicalDevice();
        VkSurfaceKHR surface = m_context.getSurface();

        // 1. Query the base capabilities of the physical monitor (min/max resolution)
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        // Fetch all color formats the monitor supports
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        // Fetch all presentation modes the monitor supports (V-sync, Mailbox, etc.)
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

        // 2. Negotiate the best available settings using our helper functions
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        VkExtent2D extent = chooseSwapExtent(capabilities);

        // 3. Determine the number of images in the swapchain queue.
        // We request minImageCount + 1 to implement Triple Buffering, reducing latency.
        uint32_t imageCount = capabilities.minImageCount + 1;
        
        // Ensure we don't exceed the maximum number of images the GPU allows
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        // 4. Configure the Swapchain creation info block
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // Always 1 unless developing stereoscopic 3D (VR)
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // We render directly to this image

        // Assume the graphics and present queues are the exact same lane.
        // This is extremely common on Intel/NVIDIA single-GPU laptop setups.
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Apply no special transforms (like rotating the screen 90 degrees)
        createInfo.preTransform = capabilities.currentTransform;
        
        // Ignore window transparency (opaque background)
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 
        createInfo.presentMode = presentMode;
        
        // Allow the GPU to discard rendering math for pixels hidden behind other OS windows
        createInfo.clipped = VK_TRUE; 
        
        // Required if resizing the window, but null for initial setup
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        // Execute the creation
        if (vkCreateSwapchainKHR(m_context.getDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create Vulkan Swapchain!");
        }

        // 5. Retrieve the raw image memory buffers created by the GPU
        vkGetSwapchainImagesKHR(m_context.getDevice(), m_swapchain, &imageCount, nullptr);
        m_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_context.getDevice(), m_swapchain, &imageCount, m_images.data());

        // Cache the format and resolution for the renderer to use later
        m_imageFormat = surfaceFormat.format;
        m_extent = extent;
        
        std::cout << "Swapchain created with " << imageCount << " images at " << extent.width << "x" << extent.height << "\n";
    }

    void Swapchain::createImageViews() {
        // Resize our view array to perfectly match the number of raw images
        m_imageViews.resize(m_images.size());

        for (size_t i = 0; i < m_images.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_imageFormat;

            // Map the colors standardly (R to R, G to G, etc.)
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // Define the purpose of the image: It holds color data, has no mipmaps, and has 1 layer
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_context.getDevice(), &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Critical Error: Failed to create Vulkan Image Views!");
            }
        }
    }

    VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        // We explicitly look for a 32-bit standard SRGB color format for accurate AAA lighting
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        // Fallback to the very first format the driver offers if SRGB is missing
        return availableFormats[0]; 
    }

    VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // Mailbox is the AAA standard (Triple Buffering). It provides the lowest latency
        // possible while still completely eliminating screen tearing.
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        // FIFO is essentially V-Sync. It is guaranteed to be supported by every Vulkan driver.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // If the window manager enforces a specific resolution, we must use it
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            // Otherwise, we calculate the exact resolution of our GLFW window
            int width, height;
            glfwGetFramebufferSize(m_window.getNativeWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // Clamp our requested resolution to the hardware's strict min/max limits
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

} // namespace Engine::Graphics