#include "SceneLoader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace Engine::Game {

    std::vector<SceneEntity> SceneLoader::loadScene(const std::string& filepath) {
        std::vector<SceneEntity> entities;
        std::ifstream file(filepath);
        
        if (!file.is_open()) {
            throw std::runtime_error("Scene file not found: " + filepath);
        }

        std::string line;
        SceneEntity currentEntity;
        bool parsingEntity = false;

        // Initialize default transform safely
        currentEntity.transform.position = glm::vec3(0.0f);
        currentEntity.transform.rotation = glm::vec3(0.0f);
        currentEntity.transform.scale = glm::vec3(1.0f);

        while (std::getline(file, line)) {
            line = trim(line);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            // Detect a new entity block
            if (line == "[Entity]") {
                // If we were already building an entity, push it to the list before starting a new one
                if (parsingEntity && !currentEntity.modelPath.empty()) {
                    entities.push_back(currentEntity);
                }
                parsingEntity = true;
                currentEntity.modelPath = "";
                currentEntity.transform.position = glm::vec3(0.0f);
                currentEntity.transform.rotation = glm::vec3(0.0f);
                currentEntity.transform.scale = glm::vec3(1.0f);
                continue;
            }

            if (parsingEntity) {
                auto delimiterPos = line.find('=');
                if (delimiterPos != std::string::npos) {
                    std::string key = trim(line.substr(0, delimiterPos));
                    std::string value = trim(line.substr(delimiterPos + 1));

                    if (key == "model") {
                        currentEntity.modelPath = value;
                    } else if (key == "position") {
                        currentEntity.transform.position = parseVec3(value);
                    } else if (key == "rotation") {
                        currentEntity.transform.rotation = parseVec3(value);
                    } else if (key == "scale") {
                        currentEntity.transform.scale = parseVec3(value);
                    }
                }
            }
        }

        // Push the very last entity in the file
        if (parsingEntity && !currentEntity.modelPath.empty()) {
            entities.push_back(currentEntity);
        }

        std::cout << "Parsed Scene: " << filepath << " (" << entities.size() << " entities found)\n";
        return entities;
    }

    glm::vec3 SceneLoader::parseVec3(const std::string& valueStr) const {
        glm::vec3 result(0.0f);
        std::stringstream ss(valueStr);
        std::string token;
        int index = 0;

        // Safely split the string by commas and convert to floats
        while (std::getline(ss, token, ',') && index < 3) {
            try {
                result[index] = std::stof(trim(token));
            } catch (...) {
                std::cerr << "[Warning] Invalid number format in scene file: " << token << " (Defaulting to 0.0)\n";
                result[index] = 0.0f;
            }
            index++;
        }
        return result;
    }

    std::string SceneLoader::trim(const std::string& str) const {
        auto start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return ""; 
        auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

} // namespace Engine::Game