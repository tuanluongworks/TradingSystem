#include "JsonParser.h"
#include <algorithm>
#include <cctype>
#include <iomanip>

std::string JsonParser::extractString(const std::string& json, const std::string& key) {
    std::string value = extractValue(json, key);
    
    // Remove quotes if present
    if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
    }
    
    return value;
}

double JsonParser::extractNumber(const std::string& json, const std::string& key) {
    std::string value = extractValue(json, key);
    
    try {
        return std::stod(value);
    } catch (...) {
        return 0.0;
    }
}

bool JsonParser::extractBool(const std::string& json, const std::string& key) {
    std::string value = extractValue(json, key);
    value = trim(value);
    
    return value == "true";
}

std::string JsonParser::createObject(const std::map<std::string, std::string>& values) {
    std::ostringstream oss;
    oss << "{";
    
    bool first = true;
    for (const auto& pair : values) {
        if (!first) oss << ", ";
        oss << "\"" << escapeString(pair.first) << "\": ";
        
        // Check if value is already JSON (starts with { or [ or is a number/bool)
        std::string trimmedValue = trim(pair.second);
        if (trimmedValue.empty()) {
            oss << "null";
        } else if (trimmedValue[0] == '{' || trimmedValue[0] == '[' || 
                   trimmedValue == "true" || trimmedValue == "false" || 
                   trimmedValue == "null" ||
                   std::all_of(trimmedValue.begin(), trimmedValue.end(), 
                              [](char c) { return std::isdigit(c) || c == '.' || c == '-'; })) {
            oss << trimmedValue;
        } else {
            oss << "\"" << escapeString(trimmedValue) << "\"";
        }
        
        first = false;
    }
    
    oss << "}";
    return oss.str();
}

std::string JsonParser::createArray(const std::vector<std::string>& items) {
    std::ostringstream oss;
    oss << "[";
    
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << items[i];
    }
    
    oss << "]";
    return oss.str();
}

std::string JsonParser::escapeString(const std::string& str) {
    std::ostringstream oss;
    
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c >= 0x20 && c <= 0x7E) {
                    oss << c;
                } else {
                    // Unicode escape
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                }
        }
    }
    
    return oss.str();
}

std::string JsonParser::extractValue(const std::string& json, const std::string& key) {
    // Find the key in the JSON
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    
    if (keyPos == std::string::npos) {
        return "";
    }
    
    // Find the colon after the key
    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) {
        return "";
    }
    
    // Find the start of the value
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && std::isspace(json[valueStart])) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        return "";
    }
    
    // Determine the end of the value
    size_t valueEnd = valueStart;
    if (json[valueStart] == '"') {
        // String value
        valueEnd = valueStart + 1;
        while (valueEnd < json.length()) {
            if (json[valueEnd] == '"' && json[valueEnd - 1] != '\\') {
                valueEnd++;
                break;
            }
            valueEnd++;
        }
    } else if (json[valueStart] == '{' || json[valueStart] == '[') {
        // Object or array - find matching bracket
        char openBracket = json[valueStart];
        char closeBracket = (openBracket == '{') ? '}' : ']';
        int bracketCount = 1;
        valueEnd = valueStart + 1;
        
        while (valueEnd < json.length() && bracketCount > 0) {
            if (json[valueEnd] == openBracket) bracketCount++;
            else if (json[valueEnd] == closeBracket) bracketCount--;
            valueEnd++;
        }
    } else {
        // Number, boolean, or null
        while (valueEnd < json.length() && 
               json[valueEnd] != ',' && 
               json[valueEnd] != '}' && 
               json[valueEnd] != ']' &&
               !std::isspace(json[valueEnd])) {
            valueEnd++;
        }
    }
    
    return json.substr(valueStart, valueEnd - valueStart);
}

std::string JsonParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
} 