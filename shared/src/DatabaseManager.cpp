#include "DatabaseManager.h"
#include <iostream>
#include <dotenv.h>

using namespace std;

unique_ptr<DatabaseManager> DatabaseManager::instance = nullptr;
mutex DatabaseManager::mutex_;

DatabaseManager::DatabaseManager(const string& dbPath, bool verbose) 
    : dbPath(dbPath), verbose(verbose), dbconn(nullptr) {
    dotenv::init("../.env");
}

DatabaseManager::~DatabaseManager() {
    disconnectFromDatabase();
}

DatabaseManager* DatabaseManager::getInstance(const string& dbPath, bool verbose) {
    lock_guard<mutex> lock(mutex_);
    if (instance == nullptr) {
        instance = unique_ptr<DatabaseManager>(new DatabaseManager(dbPath, verbose));
        instance->connectToDatabase();
        instance->initializeTables();
    }
    return instance.get();
}

bool DatabaseManager::connectToDatabase() {
    try {
        if (dbconn) {
            delete dbconn;
        }
        
        dbconn = new pqxx::connection(dbPath);
        if (dbconn->is_open() && verbose) {
            cout << "Connected to database successfully." << endl;
        }
        return dbconn->is_open();
    }
    catch (const pqxx::sql_error& e) {
        cerr << "SQL error: " << e.what() << endl;
        return false;
    }
    catch (const exception& e) {
        cerr << "Database connection error: " << e.what() << endl;
        return false;
    }
}

void DatabaseManager::disconnectFromDatabase() {
    lock_guard<mutex> lock(db_mutex);
    try {
        if (dbconn && dbconn->is_open()) {
            dbconn->close();
            if (verbose) {
                cout << "Disconnected from database successfully." << endl;
            }
        }
        delete dbconn;
        dbconn = nullptr;
    }
    catch (const exception& e) {
        cerr << "Error during disconnect: " << e.what() << endl;
    }
}

bool DatabaseManager::isConnected() const {
    return dbconn && dbconn->is_open();
}

pqxx::result DatabaseManager::executeQuery(const string& query) {
    lock_guard<mutex> lock(db_mutex);
    try {
        if (!isConnected()) {
            throw runtime_error("Database connection is not open.");
        }
        pqxx::work txn(*dbconn);
        pqxx::result result = txn.exec(query);
        txn.commit();
        return result;
    }
    catch (const exception& e) {
        cerr << "Query execution error: " << e.what() << endl;
        throw;
    }
}

pqxx::result DatabaseManager::executeParamQuery(const string& query, const vector<string>& params) {
    lock_guard<mutex> lock(db_mutex);
    try {
        if (!isConnected()) {
            throw runtime_error("Database connection is not open.");
        }
        pqxx::work txn(*dbconn);
        pqxx::result result;
        
        switch (params.size()) {
            case 1:
                result = txn.exec_params(query, params[0]);
                break;
            case 2:
                result = txn.exec_params(query, params[0], params[1]);
                break;
            case 3:
                result = txn.exec_params(query, params[0], params[1], params[2]);
                break;
            case 4:
                result = txn.exec_params(query, params[0], params[1], params[2], params[3]);
                break;
            case 5:
                result = txn.exec_params(query, params[0], params[1], params[2], params[3], params[4]);
                break;
            case 6:
                result = txn.exec_params(query, params[0], params[1], params[2], params[3], params[4], params[5]);
                break;
            default:
                throw runtime_error("Too many parameters for query");
        }
        
        txn.commit();
        return result;
    }
    catch (const exception& e) {
        cerr << "Parameterized query execution error: " << e.what() << endl;
        throw;
    }
}

bool DatabaseManager::executeUpdate(const string& query) {
    try {
        executeQuery(query);
        return true;
    }
    catch (const exception& e) {
        cerr << "Update execution error: " << e.what() << endl;
        return false;
    }
}

bool DatabaseManager::executeParamUpdate(const string& query, const vector<string>& params) {
    try {
        executeParamQuery(query, params);
        return true;
    }
    catch (const exception& e) {
        cerr << "Parameterized update execution error: " << e.what() << endl;
        return false;
    }
}

bool DatabaseManager::initializeTables() {
    try {
        string createUsersTable = R"(
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                username VARCHAR(255) UNIQUE NOT NULL,
                password VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";
        
        string createMessagesTable = R"(
            CREATE TABLE IF NOT EXISTS messages (
                id SERIAL PRIMARY KEY,
                sender VARCHAR(255) NOT NULL,
                recipient VARCHAR(255) NOT NULL,
                message_content TEXT NOT NULL,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                conversation_id VARCHAR(511) NOT NULL,
                delivered BOOLEAN DEFAULT true
            )
        )";
        
        string createConversationIndex = R"(
            CREATE INDEX IF NOT EXISTS idx_conversation_id ON messages(conversation_id);
            CREATE INDEX IF NOT EXISTS idx_timestamp ON messages(timestamp);
            CREATE INDEX IF NOT EXISTS idx_recipient_delivered ON messages(recipient, delivered);
        )";
        
        string addDeliveredColumn = R"(
            ALTER TABLE messages ADD COLUMN IF NOT EXISTS delivered BOOLEAN DEFAULT true;
        )";
        
        executeUpdate(createUsersTable);
        executeUpdate(createMessagesTable);
        executeUpdate(addDeliveredColumn);
        executeUpdate(createConversationIndex);
        
        if (verbose) {
            cout << "Database tables initialized successfully." << endl;
        }
        return true;
    }
    catch (const exception& e) {
        cerr << "Error initializing tables: " << e.what() << endl;
        return false;
    }
}

pqxx::connection* DatabaseManager::getConnection() {
    return dbconn;
}
