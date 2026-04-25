#include "ModelLoader.hpp"
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace Engine::Graphics {

    ModelLoader::~ModelLoader() {
        clearCache();
    }

    void ModelLoader::clearCache() {
        m_assetCache.clear();
        std::cout << "Model cache (RAM) cleared safely.\n";
    }

    std::shared_ptr<Model> ModelLoader::loadModel(const std::string& filepath) {
        if (m_assetCache.find(filepath) != m_assetCache.end()) {
            return m_assetCache[filepath]; 
        }

        cgltf_options options = {};
        cgltf_data* data = nullptr;
        
        cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
        if (result != cgltf_result_success) {
            throw std::runtime_error("Failed to parse GLTF/GLB file: " + filepath);
        }

        result = cgltf_load_buffers(&options, data, filepath.c_str());
        if (result != cgltf_result_success) {
            cgltf_free(data);
            throw std::runtime_error("Failed to load GLTF buffers for: " + filepath);
        }

        auto model = std::make_shared<Model>();
        model->name = filepath;

        for (cgltf_size m = 0; m < data->meshes_count; ++m) {
            cgltf_mesh* cgltfMesh = &data->meshes[m];

            for (cgltf_size p = 0; p < cgltfMesh->primitives_count; ++p) {
                cgltf_primitive* primitive = &cgltfMesh->primitives[p];
                Mesh newMesh;

                cgltf_size vertexCount = 0;
                for (cgltf_size a = 0; a < primitive->attributes_count; ++a) {
                    if (primitive->attributes[a].type == cgltf_attribute_type_position) {
                        vertexCount = primitive->attributes[a].data->count;
                        break;
                    }
                }
                newMesh.vertices.resize(vertexCount);

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

                for (auto& vert : newMesh.vertices) {
                    if (glm::length(vert.normal) < 0.01f) vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    if (glm::length(vert.color) < 0.01f)  vert.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                }

                // Extract embedded texture binary data
                if (primitive->material && primitive->material->has_pbr_metallic_roughness) {
                    cgltf_texture_view& texView = primitive->material->pbr_metallic_roughness.base_color_texture;
                    if (texView.texture && texView.texture->image && texView.texture->image->buffer_view) {
                        cgltf_image* image = texView.texture->image;
                        uint8_t* imgData = (uint8_t*)image->buffer_view->buffer->data + image->buffer_view->offset;
                        size_t imgSize = image->buffer_view->size;
                        newMesh.rawTextureData.assign(imgData, imgData + imgSize);
                    }
                }

                if (primitive->indices != nullptr) {
                    cgltf_size indexCount = primitive->indices->count;
                    newMesh.indices.resize(indexCount);
                    for (cgltf_size i = 0; i < indexCount; ++i) {
                        newMesh.indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(primitive->indices, i));
                    }
                } else {
                    newMesh.indices.resize(vertexCount);
                    for (uint32_t i = 0; i < vertexCount; ++i) newMesh.indices[i] = i;
                }

                model->meshes.push_back(newMesh);
            }
        }

        cgltf_free(data);
        m_assetCache[filepath] = model;

        std::cout << "Successfully Extracted Model: " << filepath << " (" << model->meshes.size() << " sub-meshes)\n";
        return model;
    }

} // namespace Engine::Graphics