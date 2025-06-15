#include "Config.h"
#include <fstream>
#include <iostream>
#include <string>

Config::Config(const std::string& configFilePath) : configFilePath(configFilePath) {
    loadConfig();
}

void Config::loadConfig() {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Could not open the config file: " << configFilePath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        // Process each line of the config file
        // For example, you could parse key-value pairs here
    }

    configFile.close();
}

std::string Config::getSetting(const std::string& key) const {
    // Return the value associated with the given key
    // This is a placeholder implementation
    return "";
}