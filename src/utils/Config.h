#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>

class Config {
public:
    Config(const std::string& configFilePath);
    std::string getValue(const std::string& key) const;
    void loadConfig();

private:
    std::string configFilePath;
    std::unordered_map<std::string, std::string> configValues;
};

#endif // CONFIG_H