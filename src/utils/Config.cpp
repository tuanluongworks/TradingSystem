#include "Config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <memory>

std::unique_ptr<Config> Config::global_ = nullptr;
std::mutex Config::globalMutex_;

Config::Config(const std::string& configFilePath) : configFilePath(configFilePath) { loadConfig(); }

void Config::loadConfig() {
    configValues = defaults_; // start with defaults
    std::ifstream configFile(configFilePath);
    if (configFile.is_open()) {
        std::string line; std::string currentSection;
        while (std::getline(configFile, line)) {
            line.erase(0, line.find_first_not_of(" \t")); line.erase(line.find_last_not_of(" \t") + 1);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            if (line[0] == '[' && line.back() == ']') { currentSection = line.substr(1, line.length()-2); continue; }
            size_t equalPos = line.find('='); if (equalPos != std::string::npos) { std::string key = line.substr(0, equalPos); std::string value = line.substr(equalPos + 1); key.erase(0, key.find_first_not_of(" \t")); key.erase(key.find_last_not_of(" \t") + 1); value.erase(0, value.find_first_not_of(" \t")); value.erase(value.find_last_not_of(" \t") + 1); if (value.size()>=2 && value.front()=='"' && value.back()=='"') value = value.substr(1, value.size()-2); if(!currentSection.empty()) key = currentSection + "." + key; configValues[key] = value; }
        }
        configFile.close();
    }
    mergeEnvOverrides();
    // derive dynamic settings
    dynamic_.rateLimitCapacity.store(std::stoi(configValues["rate.limit.capacity"]));
    dynamic_.rateLimitRefill.store(std::stod(configValues["rate.limit.refill_per_sec"]));
    std::string lvl = configValues["logging.level"]; int lvlInt = 1; if (lvl=="DEBUG") lvlInt=0; else if(lvl=="INFO") lvlInt=1; else if(lvl=="WARN") lvlInt=2; else if(lvl=="ERROR") lvlInt=3; dynamic_.logLevel.store(lvlInt);
}

void Config::mergeEnvOverrides() {
    if(const char* p = std::getenv("SERVER_PORT")) configValues["server.port"] = p;
    if(const char* p = std::getenv("LOG_LEVEL")) configValues["logging.level"] = p;
    if(const char* p = std::getenv("RATE_LIMIT_CAPACITY")) configValues["rate.limit.capacity"] = p;
    if(const char* p = std::getenv("RATE_LIMIT_REFILL")) configValues["rate.limit.refill_per_sec"] = p;
}

Config& Config::instance() { std::scoped_lock lock(globalMutex_); if(!global_) { global_ = std::make_unique<Config>("config/development.ini"); } return *global_; }
void Config::initialize(const std::string& path) { std::scoped_lock lock(globalMutex_); global_ = std::make_unique<Config>(path); }
std::string Config::getValue(const std::string& key) const { auto it = configValues.find(key); if (it != configValues.end()) return it->second; return ""; }