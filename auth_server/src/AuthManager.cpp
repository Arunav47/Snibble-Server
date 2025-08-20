#include "AuthManager.h"
#include <iostream>
#include <EncryptionManager.h>
#include <jwt-cpp/jwt.h>
#include <dotenv.h>
using namespace std;

AuthManager::AuthManager(bool verbose, const string& server, const string& database, const string& username, const string& password)
    : verbose(verbose) {
    dotenv::init("../.env");
    db_manager = DatabaseManager::getInstance(server, database, username, password, verbose);
    if (!db_manager || !db_manager->isConnected()) {
        cerr << "Failed to connect to database at " << server << endl;
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
        auto result = db_manager->executeParamQuery("SELECT password FROM users WHERE username = ?", params);

        if(!result.empty()) {
            string hashedPassword = result[0][0]; // First row, first column (password)
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
        return db_manager->executeParamUpdate("INSERT INTO users (username, password) VALUES (?, ?)", params);
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
        auto result = db_manager->executeParamQuery("SELECT COUNT(1) FROM users WHERE username = ?", params);

        if (!result.empty() && stoi(result[0][0]) > 0) {
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
        auto result = db_manager->executeParamQuery("SELECT TOP 10 username FROM users WHERE username LIKE ?", params);

        for (const auto& row : result) {
            userList.push_back(row[0]); // First column (username)
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
        bool result = db_manager->executeParamUpdate("UPDATE users SET public_key = ? WHERE username = ?", params);
        
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
        auto result = db_manager->executeParamQuery("SELECT public_key FROM users WHERE username = ?", params);

        if (!result.empty() && !result[0][0].empty()) {
            return result[0][0]; // First row, first column (public_key)
        }

        return "";
    }
    catch (const exception& e) {
        cerr << "getPublicKey error: " << e.what() << endl;
        return "";
    }
}