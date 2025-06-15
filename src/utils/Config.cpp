#include "Config.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Config::Config(const std::string& configFilePath) {
    loadConfig(configFilePath);
}

void Config::loadConfig(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Could not open the configuration file: " << configFilePath << std::endl;
        return;
    }

    try {
        configFile >> configData;
    } catch (const json::parse_error& e) {
        std::cerr << "Error parsing the configuration file: " << e.what() << std::endl;
    }
}

std::string Config::getValue(const std::string& key) const {
    if (configData.contains(key)) {
        return configData[key].get<std::string>();
    }
    return "";
}