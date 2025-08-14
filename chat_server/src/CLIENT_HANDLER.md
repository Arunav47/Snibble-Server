# CLIENT_HANDLER

**This documentation is for the functions of the ClientHandler class if ever needed to change in future**

- Handles individual client connections to the chat server
- Manages client authentication and disconnection
- Integrates with Redis for message broadcasting
- Works closely with SocketServer and MessageHandler

## Constructor
- **Takes SocketServer pointer as parameter**
- **Initializes Redis connection for message operations**
- **Sets up client handling parameters**
- **Establishes reference to parent server**

## Destructor
- **Cleans up Redis connection**
- **Releases any allocated resources**
- **Ensures proper cleanup of client state**

## Connect To Redis
- **Establishes connection to Redis server**
- **Creates Redis context for publishing messages**
- **Handles Redis connection errors**
- **Sets up Redis for real-time message broadcasting**

## Client Connection Handler
- **Manages the main client connection loop**
- **Handles client authentication process**
- **Processes incoming client messages**
- **Maintains client session state**
- **Coordinates with MessageHandler for message processing**
- **Updates server's client maps with user information**

## Client Disconnect Handler
- **Handles client disconnection cleanup**
- **Removes client from server's active client maps**
- **Updates user online/offline status**
- **Closes client socket connections**
- **Publishes disconnection status to Redis**
- **Cleans up thread resources for the disconnected client**
- **Ensures proper thread termination and memory cleanup**
