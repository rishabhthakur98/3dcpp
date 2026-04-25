#pragma once

#include <string>
#include <unordered_map>

namespace Engine::Core {

    class Config {
    public:
        Config(const std::string& filepath);
        ~Config() = default;

        // Getters
        bool getBool(const std::string& key, bool fallback) const;
        int getInt(const std::string& key, int fallback) const;
        float getFloat(const std::string& key, float fallback) const;
        std::string getString(const std::string& key, const std::string& fallback) const;

        // Setters (Updates live memory)
        void setBool(const std::string& key, bool value);
        void setInt(const std::string& key, int value);
        void setFloat(const std::string& key, float value);

        // Saves the live memory back to the hard drive so settings persist between launches
        void save();

    private:
        std::string m_filepath;
        std::unordered_map<std::string, std::string> m_data;
        
        std::string trim(const std::string& str) const;
    };

} // namespace Engine::Core