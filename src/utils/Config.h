#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <optional>

struct DynamicSettings {
    std::atomic<int> rateLimitCapacity{50};
    std::atomic<double> rateLimitRefill{10.0};
    std::atomic<int> logLevel{1};
};

class Config {
public:
    Config(const std::string& configFilePath);
    std::string getValue(const std::string& key) const;
    void loadConfig();
    static Config& instance();
    static void initialize(const std::string& path);
    const DynamicSettings& dynamic() const { return dynamic_; }
    DynamicSettings& dynamic() { return dynamic_; }

private:
    std::string configFilePath;
    std::unordered_map<std::string, std::string> configValues; // merged
    std::unordered_map<std::string, std::string> defaults_ {
        {"server.port", "8080"},
        {"logging.level", "INFO"},
        {"rate.limit.capacity", "50"},
        {"rate.limit.refill_per_sec", "10"}
    };
    void mergeEnvOverrides();
    static std::unique_ptr<Config> global_;
    static std::mutex globalMutex_;
    DynamicSettings dynamic_{}; // hot-reload subset
};

#endif // CONFIG_H