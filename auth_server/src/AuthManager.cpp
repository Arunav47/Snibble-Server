#include "AuthManager.h"
#include <iostream>
#include <EncryptionManager.h>
#include <jwt-cpp/jwt.h>
#include <dotenv.h>
using namespace std;

AuthManager::AuthManager(bool verbose, const string& dbPath)
    : verbose(verbose) {
    dotenv::init("../.env");
    db_manager = DatabaseManager::getInstance(dbPath, verbose);
    if (!db_manager || !db_manager->isConnected()) {
        cerr << "Failed to connect to database at " << dbPath << endl;
    }
}

AuthManager::~AuthManager() {
}

bool AuthManager::login(const string& username, const string& password) {
    if (this->verbose) {
        cout << "Attempting to log in user: " << username << endl;
    }

    try {
        if (!db_manager || !db_manager->isConnected()) {
            throw runtime_error("Database connection is not open.");
        }

        vector<string> params = {username};
        pqxx::result r = db_manager->executeParamQuery("SELECT * FROM users WHERE username = $1", params);

        if(!r.empty()) {
            string hashedPassword = r[0]["password"].as<string>();
            if (EncryptionManager::verifyPassword(password, hashedPassword)) {
                return true;
            }
            else{
                return false;
            }
        }

        return false;
    }
    catch (const exception& e) {
        cerr << "Login error: " << e.what() << endl;
        return false;
    }
}


/*
    Log out is not implemented as for logout I just deleted the token from the clients local storage...
*/
bool AuthManager::logout() {
    if (this->verbose) {
        cout << "Logging out user" << endl;
    }

    try {
        if (db_manager && db_manager->isConnected()) {
            
            return true;
        } else {
            throw runtime_error("Database connection is not open.");
        }
    }
    catch (const exception& e) {
        cerr << "Logout error: " << e.what() << endl;
        return false;
    }
}

bool AuthManager::signup(const string& username, const string& password) {
    try {
        if (!db_manager || !db_manager->isConnected()) {
            cerr << "Database connection is not open." << endl;
            return false;
        }

        string hashedPassword = EncryptionManager::hashPassword(password);

        if(hashedPassword.empty()) {
            cerr << "Failed to hash password." << endl;
            return false;
        }

        vector<string> params = {username, hashedPassword};
        return db_manager->executeParamUpdate("INSERT INTO users (username, password) VALUES ($1, $2)", params);
    }
    catch (const exception& e) {
        cerr << "Signup error: " << e.what() << endl;
        return false;
    }
}

bool AuthManager::isUserRegistered(const string& username) {
    try {
        if (!db_manager || !db_manager->isConnected()) {
            throw runtime_error("Database connection is not open.");
        }

        vector<string> params = {username};
        pqxx::result res = db_manager->executeParamQuery("SELECT COUNT(1) FROM users WHERE username = $1", params);

        if (!res.empty() && res[0][0].as<int>() > 0) {
            return true;
        }

        return false;
    }
    catch (const exception& e) {
        cerr << "isUserRegistered error: " << e.what() << endl;
        return false;
    }
}

vector<string> AuthManager::searchUsers(const string& searchTerm) {
    vector<string> userList;
    try {
        if (!db_manager || !db_manager->isConnected()) {
            throw runtime_error("Database connection is not open.");
        }

        string searchPattern = "%" + searchTerm + "%";
        vector<string> params = {searchPattern};
        pqxx::result res = db_manager->executeParamQuery("SELECT username FROM users WHERE username ILIKE $1 LIMIT 10", params);

        for (const auto& row : res) {
            userList.push_back(row["username"].as<string>());
        }
    }
    catch (const exception& e) {
        cerr << "searchUsers error: " << e.what() << endl;
    }
    return userList;
}

bool AuthManager::storePublicKey(const string& username, const string& publicKey) {
    try {
        if (!db_manager || !db_manager->isConnected()) {
            throw runtime_error("Database connection is not open.");
        }

        if (!isUserRegistered(username)) {
            cerr << "User not registered: " << username << endl;
            return false;
        }

        vector<string> params = {publicKey, username};
        bool result = db_manager->executeParamUpdate("UPDATE users SET public_key = $1 WHERE username = $2", params);
        
        if (verbose && result) {
            cout << "Stored public key for user: " << username << endl;
        }
        
        return result;
    }
    catch (const exception& e) {
        cerr << "storePublicKey error: " << e.what() << endl;
        return false;
    }
}

string AuthManager::getPublicKey(const string& username) {
    try {
        if (!db_manager || !db_manager->isConnected()) {
            throw runtime_error("Database connection is not open.");
        }

        vector<string> params = {username};
        pqxx::result res = db_manager->executeParamQuery("SELECT public_key FROM users WHERE username = $1", params);

        if (!res.empty() && !res[0]["public_key"].is_null()) {
            return res[0]["public_key"].as<string>();
        }

        return "";
    }
    catch (const exception& e) {
        cerr << "getPublicKey error: " << e.what() << endl;
        return "";
    }
}