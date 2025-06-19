#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <map>
#include <vector>
#include <sstream>

class JsonParser {
public:
    // Simple JSON value extraction (for basic use cases)
    static std::string extractString(const std::string& json, const std::string& key);
    static double extractNumber(const std::string& json, const std::string& key);
    static bool extractBool(const std::string& json, const std::string& key);
    
    // Simple JSON object creation
    static std::string createObject(const std::map<std::string, std::string>& values);
    static std::string createArray(const std::vector<std::string>& items);
    
    // Escape string for JSON
    static std::string escapeString(const std::string& str);
    
private:
    static std::string extractValue(const std::string& json, const std::string& key);
    static std::string trim(const std::string& str);
};

#endif // JSONPARSER_H 