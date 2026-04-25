#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace Engine::Graphics {

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec4 color;
        glm::vec4 tangent;

        // Tells Vulkan the size of one complete Vertex
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        // Maps the C++ variables to the HLSL [[vk::location(X)]] inputs
        static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 5> attributes{};
            
            attributes[0].binding = 0; attributes[0].location = 0;
            attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT; attributes[0].offset = offsetof(Vertex, position);
            
            attributes[1].binding = 0; attributes[1].location = 1;
            attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT; attributes[1].offset = offsetof(Vertex, normal);
            
            attributes[2].binding = 0; attributes[2].location = 2;
            attributes[2].format = VK_FORMAT_R32G32_SFLOAT; attributes[2].offset = offsetof(Vertex, uv);
            
            attributes[3].binding = 0; attributes[3].location = 3;
            attributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT; attributes[3].offset = offsetof(Vertex, color);
            
            attributes[4].binding = 0; attributes[4].location = 4;
            attributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT; attributes[4].offset = offsetof(Vertex, tangent);

            return attributes;
        }
    };

} // namespace Engine::Graphics