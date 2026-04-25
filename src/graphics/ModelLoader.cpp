#include "ModelLoader.hpp"
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace Engine::Graphics {

    ModelLoader::ModelLoader() {}

    ModelLoader::~ModelLoader() {
        clearCache();
    }

    void ModelLoader::clearCache() {
        // Safely free all CGLTF pointers in our cache
        for (auto& pair : m_assetCache) {
            if (pair.second != nullptr) {
                cgltf_free(pair.second);
            }
        }
        m_assetCache.clear();
        std::cout << "Model cache cleared safely.\n";
    }

    void ModelLoader::loadModel(const std::string& filepath) {
        // Fast Asset Re-use: Check if we already loaded this exact file
        if (m_assetCache.find(filepath) != m_assetCache.end()) {
            return; // Asset is already in RAM, instantly return
        }

        cgltf_options options = {};
        cgltf_data* data = nullptr;
        
        cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);

        // Safe failure: If a single asset fails, we throw to the GameManager, 
        // but the engine continues running.
        if (result != cgltf_result_success) {
            throw std::runtime_error("Failed to parse GLTF file: " + filepath);
        }

        result = cgltf_load_buffers(&options, data, filepath.c_str());
        if (result != cgltf_result_success) {
            cgltf_free(data);
            throw std::runtime_error("Failed to load GLTF buffers for: " + filepath);
        }

        // Store the successfully loaded model in our cache for future re-use
        m_assetCache[filepath] = data;

        std::cout << "Successfully loaded and cached model: " << filepath << '\n';
    }

} // namespace Engine::Graphics