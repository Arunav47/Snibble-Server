# DATABASE_MANAGER

**This documentation is for the functions of the DatabaseManager class if ever needed to change in future**

- Handles database connections using PostgreSQL
- Implements singleton pattern for database access
- Provides thread-safe database operations
- Manages database schema initialization

## Constructor
- **Private constructor for singleton pattern**
- **Loads environment variables using dotenv**
- **Initializes database connection parameters**

## Get Instance (Singleton)
- **Returns the singleton instance of DatabaseManager**
- **Creates new instance if it doesn't exist**
- **Automatically connects to database and initializes tables**
- **Thread-safe implementation using mutex**

## Connect To Database
- **Establishes connection to PostgreSQL database**
- **Uses connection string from dbPath parameter**
- **Returns true if connection successful, false otherwise**
- **Handles SQL errors and connection exceptions**

## Disconnect From Database
- **Safely closes database connection**
- **Thread-safe using mutex lock**
- **Cleans up connection resources**
- **Handles disconnection errors gracefully**

## Is Connected
- **Checks if database connection is active**
- **Returns true if connection is open, false otherwise**

## Execute Query
- **Executes SQL SELECT queries**
- **Thread-safe using mutex lock**
- **Returns pqxx::result with query results**
- **Handles transaction management automatically**

## Execute Param Query
- **Executes parameterized SQL SELECT queries**
- **Prevents SQL injection using prepared statements**
- **Supports up to 5 parameters**
- **Thread-safe and returns pqxx::result**

## Execute Update
- **Executes SQL INSERT, UPDATE, DELETE operations**
- **Returns true if operation successful, false otherwise**
- **Thread-safe with automatic transaction management**

## Execute Param Update
- **Executes parameterized SQL INSERT, UPDATE, DELETE operations**
- **Prevents SQL injection using prepared statements**
- **Supports up to 5 parameters**
- **Returns boolean indicating success**

## Initialize Tables
- **Creates database schema if tables don't exist**
- **Creates users table with username, password, public_key columns**
- **Creates messages table for storing chat messages**
- **Creates contacted_users table for user relationships**
- **Returns true if schema creation successful**

## Get Connection
- **Returns raw database connection pointer**
- **Should be used with caution for advanced operations**
- **Not thread-safe - caller responsible for synchronization**
