#pragma once
#include "Mesh.hpp"
#include <vector>
#include <string>

namespace Engine::Graphics {

    class Model {
    public:
        Model() = default;
        ~Model() = default;

        std::string name;
        std::vector<Mesh> meshes;
        bool isUploaded = false;
    };

} // namespace Engine::Graphics