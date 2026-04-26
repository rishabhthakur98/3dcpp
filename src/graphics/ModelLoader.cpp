#include "ModelLoader.hpp"
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace Engine::Graphics {

    ModelLoader::~ModelLoader() {
        clearCache();
    }

    void ModelLoader::clearCache() {
        // The shared_ptr automatically cleans up the Model memory!
        // The raw cgltf_data was already freed at the end of loadModel.
        m_assetCache.clear();
        std::cout << "Model cache (RAM) cleared safely.\n";
    }

    std::shared_ptr<Model> ModelLoader::loadModel(const std::string& filepath) {
        if (m_assetCache.find(filepath) != m_assetCache.end()) {
            return m_assetCache[filepath]; 
        }

        cgltf_options options = {};
        cgltf_data* data = nullptr;
        
        // Parse the physical file from the hard drive
        cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
        if (result != cgltf_result_success) {
            throw std::runtime_error("Failed to parse GLTF/GLB file: " + filepath);
        }

        // Load the binary buffers associated with the model
        result = cgltf_load_buffers(&options, data, filepath.c_str());
        if (result != cgltf_result_success) {
            cgltf_free(data);
            throw std::runtime_error("Failed to load GLTF buffers for: " + filepath);
        }

        auto model = std::make_shared<Model>();
        model->name = filepath;

        // Helper lambda to extract raw texture bytes directly from GLTF memory buffers
        auto extractTexture = [](cgltf_texture_view& view, std::vector<uint8_t>& outData) {
            if (view.texture && view.texture->image && view.texture->image->buffer_view) {
                cgltf_image* image = view.texture->image;
                uint8_t* imgData = (uint8_t*)image->buffer_view->buffer->data + image->buffer_view->offset;
                outData.assign(imgData, imgData + image->buffer_view->size);
            }
        };

        // Iterate through all sub-meshes in the GLTF file
        for (cgltf_size m = 0; m < data->meshes_count; ++m) {
            cgltf_mesh* cgltfMesh = &data->meshes[m];

            for (cgltf_size p = 0; p < cgltfMesh->primitives_count; ++p) {
                cgltf_primitive* primitive = &cgltfMesh->primitives[p];
                Mesh newMesh;

                // Determine the total vertex count for this primitive
                cgltf_size vertexCount = 0;
                for (cgltf_size a = 0; a < primitive->attributes_count; ++a) {
                    if (primitive->attributes[a].type == cgltf_attribute_type_position) {
                        vertexCount = primitive->attributes[a].data->count; break;
                    }
                }
                newMesh.vertices.resize(vertexCount);

                // Extract all vertex attributes (Position, Normal, UV, Color, Tangent)
                for (cgltf_size a = 0; a < primitive->attributes_count; ++a) {
                    cgltf_attribute* attr = &primitive->attributes[a];
                    for (cgltf_size v = 0; v < vertexCount; ++v) {
                        if (attr->type == cgltf_attribute_type_position) {
                            cgltf_accessor_read_float(attr->data, v, &newMesh.vertices[v].position.x, 3);
                        } else if (attr->type == cgltf_attribute_type_normal) {
                            cgltf_accessor_read_float(attr->data, v, &newMesh.vertices[v].normal.x, 3);
                        } else if (attr->type == cgltf_attribute_type_texcoord) {
                            cgltf_accessor_read_float(attr->data, v, &newMesh.vertices[v].uv.x, 2);
                        } else if (attr->type == cgltf_attribute_type_color) {
                            cgltf_accessor_read_float(attr->data, v, &newMesh.vertices[v].color.x, 4);
                        } else if (attr->type == cgltf_attribute_type_tangent) {
                            cgltf_accessor_read_float(attr->data, v, &newMesh.vertices[v].tangent.x, 4);
                        }
                    }
                }

                // Apply safe defaults for missing attributes to prevent shader artifacting
                for (auto& vert : newMesh.vertices) {
                    // Safe normal fallback
                    if (glm::length(vert.normal) < 0.01f) vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    
                    // Safe color fallback
                    if (glm::length(vert.color) < 0.01f)  vert.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    
                    // --- THE TANGENT PARADOX FIX ---
                    // If the GLTF is missing tangents, we dynamically generate one that 
                    // is mathematically guaranteed to be orthogonal to the normal vector.
                    if (glm::length(glm::vec3(vert.tangent)) < 0.01f) {
                        // Test against both Z and Y up-vectors
                        glm::vec3 test1 = glm::cross(vert.normal, glm::vec3(0.0f, 0.0f, 1.0f));
                        glm::vec3 test2 = glm::cross(vert.normal, glm::vec3(0.0f, 1.0f, 0.0f));
                        
                        // Pick whichever cross product resulted in a valid, non-zero vector
                        glm::vec3 validTangent = glm::length(test1) > glm::length(test2) ? test1 : test2;
                        
                        // Assign it! The '1.0f' at the end is the bitangent sign direction
                        vert.tangent = glm::vec4(glm::normalize(validTangent), 1.0f);
                    }
                }

                // Extract textures directly into our Mesh struct
                if (primitive->material) {
                    if (primitive->material->has_pbr_metallic_roughness) {
                        extractTexture(primitive->material->pbr_metallic_roughness.base_color_texture, newMesh.rawAlbedoData);
                        extractTexture(primitive->material->pbr_metallic_roughness.metallic_roughness_texture, newMesh.rawMetRoughData);
                    }
                    extractTexture(primitive->material->normal_texture, newMesh.rawNormalData);
                }

                // Process Indices
                if (primitive->indices != nullptr) {
                    newMesh.indices.resize(primitive->indices->count);
                    for (cgltf_size i = 0; i < primitive->indices->count; ++i) {
                        newMesh.indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(primitive->indices, i));
                    }
                } else {
                    // Generate sequential indices if the model doesn't provide an index buffer
                    newMesh.indices.resize(vertexCount);
                    for (uint32_t i = 0; i < vertexCount; ++i) newMesh.indices[i] = i;
                }

                model->meshes.push_back(newMesh);
            }
        }
        cgltf_free(data); // Safely free the C-struct now that we copied it to C++
        
        m_assetCache[filepath] = model;
        std::cout << "Successfully Extracted Model: " << filepath << " (" << model->meshes.size() << " sub-meshes)\n";
        return model;
    }

} // namespace Engine::Graphics