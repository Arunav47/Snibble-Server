#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <string>
#include <vector>
#include "DatabaseManager.h"

class AuthManager {
private:
    DatabaseManager* db_manager;
    bool verbose;

public:
    explicit AuthManager(bool verbose, const std::string& server, const std::string& database, const std::string& username, const std::string& password);
    ~AuthManager();

    bool login(const std::string& username, const std::string& password);
    bool logout();

    bool signup(const std::string& username, const std::string& password);

    bool isUserRegistered(const std::string& username);

    std::vector<std::string> searchUsers(const std::string& searchTerm);
    
    bool storePublicKey(const std::string& username, const std::string& publicKey);
    std::string getPublicKey(const std::string& username);
};

#endif
