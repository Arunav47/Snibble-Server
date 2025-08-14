# SOCKET_SERVER

**This documentation is for the functions of the SocketServer class if ever needed to change in future**

- Handles TCP socket server operations for chat functionality
- Manages client connections and message routing
- Integrates with Redis for real-time messaging
- Coordinates with ClientHandler and MessageHandler classes

## Constructor
- **Takes host, port, and database path parameters**
- **Initializes server socket configuration**
- **Sets up client connection maps and thread management**
- **Initializes mutex for thread synchronization**
- **Sets default maximum clients to 100**

## Destructor
- **Cleans up socket resources**
- **Stops all running threads**
- **Closes client connections gracefully**

## Start
- **Creates and binds TCP socket to specified host and port**
- **Starts listening for incoming client connections**
- **Spawns threads for accepting clients and handling messages**
- **Initializes Redis integration for message broadcasting**
- **Manages client connection lifecycle**

## Stop
- **Sets running flag to false to signal thread termination**
- **Waits for all client threads to complete**
- **Closes server socket and cleans up resources**
- **Disconnects from Redis**

## Client Management
- **Maintains map of client file descriptors to usernames**
- **Tracks online/offline status of users**
- **Manages thread pool for client connections**
- **Handles client disconnection cleanup**

## Redis Integration
- **Connects to Redis for real-time message broadcasting**
- **Publishes messages to Redis channels**
- **Subscribes to Redis channels for message delivery**
- **Maintains separate Redis contexts for publishing and subscribing**

## Thread Management
- **Uses pthread for concurrent client handling**
- **Maintains separate threads for message reading and client acceptance**
- **Implements thread-safe operations using mutex locks**
- **Manages thread lifecycle and cleanup**
