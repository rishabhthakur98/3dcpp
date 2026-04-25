#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace Engine::Core {

    Config::Config(const std::string& filepath) : m_filepath(filepath) {
        std::ifstream file(m_filepath);
        if (!file.is_open()) {
            std::cerr << "[Warning] Config file not found at " << filepath << ". Using engine defaults.\n";
            return; 
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#' || line[0] == '[') continue;
            auto delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                m_data[trim(line.substr(0, delimiterPos))] = trim(line.substr(delimiterPos + 1));
            }
        }
    }

    void Config::save() {
        std::ofstream file(m_filepath);
        if (!file.is_open()) {
            std::cerr << "[Error] Failed to save config to " << m_filepath << "\n";
            return;
        }

        file << "# AAA Engine Configuration (Auto-Generated)\n\n";
        file << "[Settings]\n";
        for (const auto& pair : m_data) {
            file << pair.first << " = " << pair.second << "\n";
        }
        std::cout << "Settings saved successfully to disk.\n";
    }

    void Config::setBool(const std::string& key, bool value) { m_data[key] = value ? "true" : "false"; }
    void Config::setInt(const std::string& key, int value) { m_data[key] = std::to_string(value); }
    void Config::setFloat(const std::string& key, float value) { m_data[key] = std::to_string(value); }

    bool Config::getBool(const std::string& key, bool fallback) const {
        auto it = m_data.find(key);
        if (it != m_data.end()) {
            std::string val = it->second;
            std::transform(val.begin(), val.end(), val.begin(), ::tolower);
            return (val == "true" || val == "1");
        }
        return fallback;
    }

    int Config::getInt(const std::string& key, int fallback) const {
        auto it = m_data.find(key);
        if (it != m_data.end()) {
            try { return std::stoi(it->second); } catch (...) {}
        }
        return fallback;
    }

    float Config::getFloat(const std::string& key, float fallback) const {
        auto it = m_data.find(key);
        if (it != m_data.end()) {
            try { return std::stof(it->second); } catch (...) {}
        }
        return fallback;
    }

    std::string Config::getString(const std::string& key, const std::string& fallback) const {
        auto it = m_data.find(key);
        if (it != m_data.end()) return it->second;
        return fallback;
    }

    std::string Config::trim(const std::string& str) const {
        auto start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return ""; 
        auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

} // namespace Engine::Core