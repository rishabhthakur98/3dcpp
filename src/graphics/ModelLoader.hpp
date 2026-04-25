#pragma once

#include "Model.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>

namespace Engine::Graphics {

    class ModelLoader {
    public:
        ModelLoader() = default;
        ~ModelLoader();

        ModelLoader(const ModelLoader&) = delete;
        ModelLoader& operator=(const ModelLoader&) = delete;

        // Returns a shared pointer so multiple identical props in the world 
        // safely share the exact same block of RAM!
        std::shared_ptr<Model> loadModel(const std::string& filepath);

        void clearCache();

    private:
        std::unordered_map<std::string, std::shared_ptr<Model>> m_assetCache;
    };

} // namespace Engine::Graphics