#include "SceneLoader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace Engine::Scene {

    std::vector<SceneEntity> SceneLoader::loadScene(const std::string& filepath) {
        std::vector<SceneEntity> entities;
        std::ifstream file(filepath);
        if (!file.is_open()) throw std::runtime_error("Scene file not found: " + filepath);

        std::string line;
        SceneEntity currentEntity;
        bool parsingEntity = false;

        auto resetEntity = [&]() {
            currentEntity.modelPath = "";
            currentEntity.transform.position = glm::vec3(0.0f);
            currentEntity.transform.rotation = glm::vec3(0.0f);
            currentEntity.transform.scale = glm::vec3(1.0f);
            currentEntity.props = EntityProperties(); // Reset to defaults
        };
        resetEntity();

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            if (line == "[Entity]") {
                if (parsingEntity && !currentEntity.modelPath.empty()) entities.push_back(currentEntity);
                parsingEntity = true;
                resetEntity();
                continue;
            }

            if (parsingEntity) {
                auto delimiterPos = line.find('=');
                if (delimiterPos != std::string::npos) {
                    std::string key = trim(line.substr(0, delimiterPos));
                    std::string value = trim(line.substr(delimiterPos + 1));

                    if (key == "model") currentEntity.modelPath = value;
                    else if (key == "position") currentEntity.transform.position = parseVec3(value);
                    else if (key == "rotation") currentEntity.transform.rotation = parseVec3(value);
                    else if (key == "scale") currentEntity.transform.scale = parseVec3(value);
                    // --- NEW: Parsing the advanced properties ---
                    else if (key == "tint") currentEntity.props.tint = parseVec3(value);
                    else if (key == "visible") currentEntity.props.visible = parseBool(value);
                    else if (key == "cast_shadows") currentEntity.props.castShadows = parseBool(value);
                    else if (key == "mass") currentEntity.props.mass = parseFloat(value);
                    else if (key == "tag") currentEntity.props.tag = value;
                }
            }
        }
        if (parsingEntity && !currentEntity.modelPath.empty()) entities.push_back(currentEntity);

        std::cout << "Parsed Scene: " << filepath << " (" << entities.size() << " entities found)\n";
        return entities;
    }

    glm::vec3 SceneLoader::parseVec3(const std::string& valueStr) const {
        glm::vec3 result(0.0f);
        std::stringstream ss(valueStr);
        std::string token;
        int index = 0;
        while (std::getline(ss, token, ',') && index < 3) {
            try { result[index] = std::stof(trim(token)); } catch (...) { result[index] = 0.0f; }
            index++;
        }
        return result;
    }

    bool SceneLoader::parseBool(const std::string& valueStr) const {
        std::string lower = valueStr;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "true" || lower == "1";
    }

    float SceneLoader::parseFloat(const std::string& valueStr) const {
        try { return std::stof(valueStr); } catch (...) { return 0.0f; }
    }

    std::string SceneLoader::trim(const std::string& str) const {
        auto start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return ""; 
        auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

} // namespace Engine::Scene