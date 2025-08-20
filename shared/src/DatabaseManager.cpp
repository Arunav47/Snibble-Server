#include "DatabaseManager.h"
#include <iostream>
#include <dotenv.h>
#include <cstring>

using namespace std;

unique_ptr<DatabaseManager> DatabaseManager::instance = nullptr;
mutex DatabaseManager::mutex_;

DatabaseManager::DatabaseManager(const string& server, const string& database, const string& username, const string& password, bool verbose) 
    : verbose(verbose), hEnv(SQL_NULL_HENV), hDbc(SQL_NULL_HDBC), hStmt(SQL_NULL_HSTMT) {
    // Build Azure SQL connection string with proper Azure-specific parameters
    connectionString = "DRIVER={ODBC Driver 18 for SQL Server};"
                      "SERVER=tcp:" + server + ",1433;"
                      "DATABASE=" + database + ";"
                      "UID=" + username + ";"
                      "PWD=" + password + ";"
                      "Encrypt=yes;"
                      "TrustServerCertificate=no;"
                      "Connection Timeout=30;"
                      "Authentication=SqlPassword;"
                      "LoginTimeout=30;";
    
    if (verbose) {
        cout << "Connection string built for Azure SQL Database" << endl;
        // Don't log the actual connection string as it contains password
    }
}

DatabaseManager::~DatabaseManager() {
    disconnectFromDatabase();
}

DatabaseManager* DatabaseManager::getInstance(const string& server, const string& database, const string& username, const string& password, bool verbose) {
    lock_guard<mutex> lock(mutex_);
    if (instance == nullptr) {
        instance = unique_ptr<DatabaseManager>(new DatabaseManager(server, database, username, password, verbose));
        instance->connectToDatabase();
        instance->initializeTables();
    }
    return instance.get();
}

bool DatabaseManager::connectToDatabase() {
    try {
        // Allocate environment handle
        lock_guard<mutex> lock(db_mutex);
        if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) != SQL_SUCCESS) {
            if(verbose) {
                cerr << "Failed to allocate SQL environment handle." << endl;
            }
            return false;
        }
        
        // Set ODBC version
        if (SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0) != SQL_SUCCESS) {
            if(verbose) {
                cerr << "Failed to set ODBC version." << endl;
            }
            return false;
        }
        
        // Allocate connection handle
        if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS) {
            if(verbose) {
                cerr << "Failed to allocate connection handle." << endl;
            }
            return false;
        }
        
        if (verbose) {
            cout << "Attempting to connect with connection string: " << endl;
        }
        
        // Connect to Azure SQL Database
        SQLCHAR outConnectionString[1024];
        SQLSMALLINT outConnectionStringLength;
        SQLRETURN ret = SQLDriverConnect(hDbc, nullptr, (SQLCHAR*)connectionString.c_str(), 
                                        SQL_NTS, outConnectionString, sizeof(outConnectionString), 
                                        &outConnectionStringLength, SQL_DRIVER_COMPLETE);
        
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            if (verbose) {
                cout << "Connected to Azure SQL Database successfully" << endl;
            }
            return true;
        } else {
            if(verbose) {
                cerr << "Failed to connect to Azure SQL Database." << endl;
                
                // Get detailed error information
                SQLCHAR sqlState[6];
                SQLCHAR errorMessage[SQL_MAX_MESSAGE_LENGTH];
                SQLINTEGER nativeError;
                SQLSMALLINT messageLength;
                
                if (SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, 1, sqlState, &nativeError, 
                                 errorMessage, sizeof(errorMessage), &messageLength) == SQL_SUCCESS) {
                    cerr << "SQL State: " << sqlState << endl;
                    cerr << "Native Error: " << nativeError << endl;
                    cerr << "Error Message: " << errorMessage << endl;
                }
            }
            return false;
        }
    }
    catch (const exception& e) {
        cerr << "Database connection error: " << e.what() << endl;
        return false;
    }
}

void DatabaseManager::disconnectFromDatabase() {
    lock_guard<mutex> lock(db_mutex);
    try {
        if (hStmt != SQL_NULL_HSTMT) {
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            hStmt = SQL_NULL_HSTMT;
        }
        if (hDbc != SQL_NULL_HDBC) {
            SQLDisconnect(hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            hDbc = SQL_NULL_HDBC;
        }
        if (hEnv != SQL_NULL_HENV) {
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
            hEnv = SQL_NULL_HENV;
        }
        if (verbose) {
            cout << "Disconnected from database successfully." << endl;
        }
    }
    catch (const exception& e) {
        cerr << "Error during disconnect: " << e.what() << endl;
    }
}

bool DatabaseManager::isConnected() const {
    if (hDbc == SQL_NULL_HDBC) return false;
    
    SQLUINTEGER connectionDead;
    SQLRETURN ret = SQLGetConnectAttr(hDbc, SQL_ATTR_CONNECTION_DEAD, &connectionDead, 0, nullptr);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) && (connectionDead == SQL_CD_FALSE);
}

vector<vector<string>> DatabaseManager::executeQuery(const string& query) {
    lock_guard<mutex> lock(db_mutex);
    vector<vector<string>> results;
    
    try {
        if (!isConnected()) {
            throw runtime_error("Database connection is not open.");
        }
        
        SQLHSTMT stmt;
        if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &stmt) != SQL_SUCCESS) {
            throw runtime_error("Failed to allocate statement handle.");
        }
        
        if (SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
            // Get detailed error information
            SQLCHAR sqlState[6];
            SQLCHAR errorMessage[SQL_MAX_MESSAGE_LENGTH];
            SQLINTEGER nativeError;
            SQLSMALLINT messageLength;
            
            if (SQLGetDiagRec(SQL_HANDLE_STMT, stmt, 1, sqlState, &nativeError, 
                             errorMessage, sizeof(errorMessage), &messageLength) == SQL_SUCCESS) {
                cerr << "SQL Query Error - State: " << sqlState << ", Message: " << errorMessage << endl;
            }
            
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw runtime_error("Failed to execute query: " + query);
        }
        
        SQLSMALLINT columnCount;
        SQLNumResultCols(stmt, &columnCount);
        
        SQLCHAR buffer[1024];
        SQLLEN indicator;
        
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            vector<string> row;
            for (SQLSMALLINT i = 1; i <= columnCount; i++) {
                if (SQLGetData(stmt, i, SQL_C_CHAR, buffer, sizeof(buffer), &indicator) == SQL_SUCCESS) {
                    if (indicator == SQL_NULL_DATA) {
                        row.push_back("");
                    } else {
                        row.push_back(string((char*)buffer));
                    }
                } else {
                    row.push_back("");
                }
            }
            results.push_back(row);
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return results;
    }
    catch (const exception& e) {
        cerr << "Query execution error: " << e.what() << endl;
        throw;
    }
}

vector<vector<string>> DatabaseManager::executeParamQuery(const string& query, const vector<string>& params) {
    lock_guard<mutex> lock(db_mutex);
    vector<vector<string>> results;
    
    try {
        if (!isConnected()) {
            throw runtime_error("Database connection is not open.");
        }
        
        SQLHSTMT stmt;
        if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &stmt) != SQL_SUCCESS) {
            throw runtime_error("Failed to allocate statement handle.");
        }
        
        if (SQLPrepare(stmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw runtime_error("Failed to prepare query.");
        }
        
        // Bind parameters
        for (size_t i = 0; i < params.size(); i++) {
            if (SQLBindParameter(stmt, i + 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                               params[i].length(), 0, (SQLCHAR*)params[i].c_str(),
                               params[i].length(), nullptr) != SQL_SUCCESS) {
                SQLFreeHandle(SQL_HANDLE_STMT, stmt);
                throw runtime_error("Failed to bind parameter " + to_string(i + 1));
            }
        }
        
        if (SQLExecute(stmt) != SQL_SUCCESS) {
            // Get detailed error information
            SQLCHAR sqlState[6];
            SQLCHAR errorMessage[SQL_MAX_MESSAGE_LENGTH];
            SQLINTEGER nativeError;
            SQLSMALLINT messageLength;
            
            if (SQLGetDiagRec(SQL_HANDLE_STMT, stmt, 1, sqlState, &nativeError, 
                             errorMessage, sizeof(errorMessage), &messageLength) == SQL_SUCCESS) {
                cerr << "SQL Parameterized Query Error - State: " << sqlState << ", Message: " << errorMessage << endl;
            }
            
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            throw runtime_error("Failed to execute parameterized query: " + query);
        }
        
        SQLSMALLINT columnCount;
        SQLNumResultCols(stmt, &columnCount);
        
        SQLCHAR buffer[1024];
        SQLLEN indicator;
        
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            vector<string> row;
            for (SQLSMALLINT i = 1; i <= columnCount; i++) {
                if (SQLGetData(stmt, i, SQL_C_CHAR, buffer, sizeof(buffer), &indicator) == SQL_SUCCESS) {
                    if (indicator == SQL_NULL_DATA) {
                        row.push_back("");
                    } else {
                        row.push_back(string((char*)buffer));
                    }
                } else {
                    row.push_back("");
                }
            }
            results.push_back(row);
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return results;
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
        // Convert PostgreSQL syntax to SQL Server syntax
        string createUsersTable = R"(
            IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='users' AND xtype='U')
            CREATE TABLE users (
                id INT IDENTITY(1,1) PRIMARY KEY,
                username NVARCHAR(255) UNIQUE NOT NULL,
                password NVARCHAR(255) NOT NULL,
                public_key NVARCHAR(MAX) NULL,
                created_at DATETIME2 DEFAULT GETDATE()
            )
        )";
        
        string createMessagesTable = R"(
            IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='messages' AND xtype='U')
            CREATE TABLE messages (
                id INT IDENTITY(1,1) PRIMARY KEY,
                sender NVARCHAR(255) NOT NULL,
                recipient NVARCHAR(255) NOT NULL,
                message_content NTEXT NOT NULL,
                timestamp DATETIME2 DEFAULT GETDATE(),
                conversation_id NVARCHAR(511) NOT NULL,
                delivered BIT DEFAULT 1
            )
        )";
        
        string createConversationIndex = R"(
            IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'idx_conversation_id')
                CREATE INDEX idx_conversation_id ON messages(conversation_id);
            IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'idx_timestamp')
                CREATE INDEX idx_timestamp ON messages(timestamp);
            IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name = 'idx_recipient_delivered')
                CREATE INDEX idx_recipient_delivered ON messages(recipient, delivered);
        )";
        
        string addPublicKeyColumn = R"(
            IF NOT EXISTS (SELECT * FROM sys.columns WHERE object_id = OBJECT_ID('users') AND name = 'public_key')
                ALTER TABLE users ADD public_key NVARCHAR(MAX) NULL;
        )";
        
        string addDeliveredColumn = R"(
            IF NOT EXISTS (SELECT * FROM sys.columns WHERE object_id = OBJECT_ID('messages') AND name = 'delivered')
                ALTER TABLE messages ADD delivered BIT DEFAULT 1;
        )";
        
        executeUpdate(createUsersTable);
        executeUpdate(createMessagesTable);
        executeUpdate(addPublicKeyColumn);
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

SQLHDBC DatabaseManager::getConnection() {
    return hDbc;
}
