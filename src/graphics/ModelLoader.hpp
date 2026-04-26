#pragma once

#include "Model.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <cgltf.h>

namespace Engine::Graphics {

    class ModelLoader {
    public:
        ModelLoader() = default;
        ~ModelLoader();

        ModelLoader(const ModelLoader&) = delete;
        ModelLoader& operator=(const ModelLoader&) = delete;

        std::shared_ptr<Model> loadModel(const std::string& filepath);

        void clearCache();

    private:
        // Automatically manages and frees memory when cleared!
        std::unordered_map<std::string, std::shared_ptr<Model>> m_assetCache;
    };

} // namespace Engine::Graphics