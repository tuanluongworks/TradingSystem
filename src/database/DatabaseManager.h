#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <string>
#include <vector>

class DatabaseManager {
public:
    DatabaseManager(const std::string& connectionString);
    ~DatabaseManager();

    bool connect();
    void disconnect();
    bool executeQuery(const std::string& query);
    std::vector<std::vector<std::string>> fetchResults(const std::string& query);

private:
    std::string connectionString;
    bool isConnected;
};

#endif // DATABASEMANAGER_H