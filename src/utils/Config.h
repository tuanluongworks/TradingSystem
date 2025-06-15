class Config {
public:
    Config(const std::string& configFilePath);
    void load();
    std::string get(const std::string& key) const;

private:
    std::string configFilePath;
    std::map<std::string, std::string> configData;
};