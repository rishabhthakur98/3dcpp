#include "VulkanContext.hpp"
#include <iostream>
#include <stdexcept>
#include <set>

namespace Engine::Graphics {

    VulkanContext::VulkanContext(Core::Window& window)
        : m_window(window), 
          m_instance(VK_NULL_HANDLE), 
          m_surface(VK_NULL_HANDLE),
          m_physicalDevice(VK_NULL_HANDLE),
          m_device(VK_NULL_HANDLE),
          m_allocator(VK_NULL_HANDLE) {
        
        createInstance();
        createSurface();
        queryHardware();       
        createLogicalDevice(); 
        initVMA();             
    }

    VulkanContext::~VulkanContext() {
        if (m_allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(m_allocator);
        }
        if (m_device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_device, nullptr);
        }
        if (m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
        }
    }

    void VulkanContext::createInstance() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "3D C++ AAA Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Custom Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3; 

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create Vulkan Instance!");
        }
    }

    void VulkanContext::createSurface() {
        if (glfwCreateWindowSurface(m_instance, m_window.getNativeWindow(), nullptr, &m_surface) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create Vulkan window surface!");
        }
    }

    QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
            if (presentSupport) indices.presentFamily = i;

            if (indices.isComplete()) break;
            i++;
        }
        return indices;
    }

    void VulkanContext::queryHardware() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        if (deviceCount == 0) throw std::runtime_error("Critical Error: Failed to find GPUs with Vulkan support.");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            QueueFamilyIndices indices = findQueueFamilies(device);
            if (!indices.isComplete()) continue;
            if (!checkDeviceExtensionSupport(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            // Safely query hardware limits
            VkPhysicalDeviceVulkan12Features features12{};
            features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            
            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features2.pNext = &features12;
            
            vkGetPhysicalDeviceFeatures2(device, &features2);

            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(device, &memProps);
            uint32_t vramMB = 0;
            for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
                if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    vramMB += static_cast<uint32_t>(memProps.memoryHeaps[i].size / (1024 * 1024));
                }
            }

            uint32_t ver = properties.apiVersion;
            std::string apiVer = std::to_string(VK_API_VERSION_MAJOR(ver)) + "." +
                                 std::to_string(VK_API_VERSION_MINOR(ver)) + "." +
                                 std::to_string(VK_API_VERSION_PATCH(ver));

            bool isDiscrete = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
            
            if (m_physicalDevice == VK_NULL_HANDLE || isDiscrete) {
                m_physicalDevice = device;
                m_specs.deviceName = properties.deviceName;
                m_specs.deviceType = isDiscrete ? "Discrete GPU" : "Integrated GPU";
                m_specs.apiVersion = apiVer;
                m_specs.dedicatedVRAM_MB = vramMB;
                m_specs.maxTextureResolution = properties.limits.maxImageDimension2D;
                m_specs.isDiscreteGpu = isDiscrete;
                
                m_specs.supportsHwRayTracing = checkDeviceExtensionSupport(device, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                m_specs.supportsSwRayTracing = true; 
                
                m_specs.supportsBindless = (features12.descriptorBindingPartiallyBound && features12.runtimeDescriptorArray);
                m_specs.supportsMeshShaders = checkDeviceExtensionSupport(device, VK_EXT_MESH_SHADER_EXTENSION_NAME);
                m_specs.supportsVRS = checkDeviceExtensionSupport(device, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
                m_specs.supportsMultiDrawIndirect = features2.features.multiDrawIndirect;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Critical Error: Failed to find a suitable GPU.");
        }
    }

    void VulkanContext::createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f; 
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkPhysicalDeviceVulkan12Features supportedFeatures12{};
        supportedFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        
        VkPhysicalDeviceFeatures2 supportedFeatures2{};
        supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        supportedFeatures2.pNext = &supportedFeatures12;
        vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supportedFeatures2);

        VkPhysicalDeviceVulkan12Features requestedFeatures12{};
        requestedFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        requestedFeatures12.descriptorBindingPartiallyBound = supportedFeatures12.descriptorBindingPartiallyBound;
        requestedFeatures12.runtimeDescriptorArray = supportedFeatures12.runtimeDescriptorArray;

        VkPhysicalDeviceFeatures requestedFeatures{};
        requestedFeatures.multiDrawIndirect = supportedFeatures2.features.multiDrawIndirect;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &requestedFeatures12; 
        createInfo.pEnabledFeatures = &requestedFeatures; 
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to create Vulkan Logical Device!");
        }

        vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
        
        std::cout << "Logical Device created successfully.\n";
    }

    void VulkanContext::initVMA() {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = m_physicalDevice;
        allocatorInfo.device = m_device;
        allocatorInfo.instance = m_instance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
            throw std::runtime_error("Critical Error: Failed to initialize Vulkan Memory Allocator!");
        }
        
        std::cout << "Vulkan Memory Allocator (VMA) initialized successfully.\n";
    }

    bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device, const char* extensionName) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        for (const auto& extension : availableExtensions) {
            if (std::string(extension.extensionName) == extensionName) return true;
        }
        return false;
    }

} // namespace Engine::Graphics