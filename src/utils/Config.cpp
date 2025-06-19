#include "Config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

Config::Config(const std::string& configFilePath) : configFilePath(configFilePath) {
    loadConfig();
}

void Config::loadConfig() {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Could not open the configuration file: " << configFilePath << std::endl;
        return;
    }

    std::string line;
    std::string currentSection;
    
    while (std::getline(configFile, line)) {
        // Remove whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section header
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key-value pair
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Remove quotes if present
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            // Store with section prefix
            if (!currentSection.empty()) {
                key = currentSection + "." + key;
            }
            
            configValues[key] = value;
        }
    }
    
    configFile.close();
}

std::string Config::getValue(const std::string& key) const {
    auto it = configValues.find(key);
    if (it != configValues.end()) {
        return it->second;
    }
    return "";
}