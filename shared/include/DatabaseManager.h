#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H



#include <sql.h>
#include <sqlext.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>

class DatabaseManager {
private:
    static std::unique_ptr<DatabaseManager> instance;
    static std::mutex mutex_;

    SQLHENV hEnv;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;
    std::string connectionString;
    bool verbose;
    std::mutex db_mutex;

    DatabaseManager(const std::string& server, const std::string& database, const std::string& username, const std::string& password, bool verbose = false);

public:
    ~DatabaseManager();
    
    // Singleton pattern
    static DatabaseManager* getInstance(const std::string& server, const std::string& database, const std::string& username, const std::string& password, bool verbose = false);

    // Database operations
    bool connectToDatabase();
    void disconnectFromDatabase();
    bool isConnected() const;
    
    // Thread-safe database operations
    std::vector<std::vector<std::string>> executeQuery(const std::string& query);
    std::vector<std::vector<std::string>> executeParamQuery(const std::string& query, const std::vector<std::string>& params);
    bool executeUpdate(const std::string& query);
    bool executeParamUpdate(const std::string& query, const std::vector<std::string>& params);
    
    // Schema initialization
    bool initializeTables();
    
    // Get raw connection handle (use with caution)
    SQLHDBC getConnection();
    
    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
};

#endif // DATABASEMANAGER_H
