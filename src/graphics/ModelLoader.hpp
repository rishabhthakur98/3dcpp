#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <cgltf.h> 

namespace Engine::Graphics {

    class ModelLoader {
    public:
        ModelLoader();
        ~ModelLoader();

        ModelLoader(const ModelLoader&) = delete;
        ModelLoader& operator=(const ModelLoader&) = delete;

        // Requests a model. If already loaded, returns cached version immediately.
        void loadModel(const std::string& filepath);

        // Clears all memory when switching worlds
        void clearCache();

    private:
        // A hash map to store models we have already loaded so we can reuse them safely
        std::unordered_map<std::string, cgltf_data*> m_assetCache;
    };

} // namespace Engine::Graphics