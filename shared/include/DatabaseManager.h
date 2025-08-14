#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <mutex>

class DatabaseManager {
private:
    static std::unique_ptr<DatabaseManager> instance;
    static std::mutex mutex_;
    
    pqxx::connection* dbconn;
    std::string dbPath;
    bool verbose;
    std::mutex db_mutex;

    DatabaseManager(const std::string& dbPath, bool verbose = false);

public:
    ~DatabaseManager();
    
    // Singleton pattern
    static DatabaseManager* getInstance(const std::string& dbPath = "", bool verbose = false);
    
    // Database operations
    bool connectToDatabase();
    void disconnectFromDatabase();
    bool isConnected() const;
    
    // Thread-safe database operations
    pqxx::result executeQuery(const std::string& query);
    pqxx::result executeParamQuery(const std::string& query, const std::vector<std::string>& params);
    bool executeUpdate(const std::string& query);
    bool executeParamUpdate(const std::string& query, const std::vector<std::string>& params);
    
    // Schema initialization
    bool initializeTables();
    
    // Get raw connection (use with caution)
    pqxx::connection* getConnection();
    
    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
};

#endif // DATABASEMANAGER_H
