# MESSAGE_HANDLER (Server)

**This documentation is for the functions of the MessageHandler class in the chat server if ever needed to change in future**

- Handles message storage and forwarding functionality
- Manages offline message delivery
- Provides chat history and contacted users features
- Integrates with database for persistent storage

## Constructor
- **Takes SocketServer and ClientHandler pointers as parameters**
- **Optionally takes database path for connection**
- **Initializes database connection through DatabaseManager**
- **Sets up references to server and client handler components**

## Destructor
- **Cleans up database connection references**
- **DatabaseManager cleanup handled by singleton pattern**

## Connect To Database
- **Establishes connection to PostgreSQL database**
- **Uses DatabaseManager singleton for connection management**
- **Handles database connection errors**

## Store Message In Database
- **Stores chat messages persistently in database**
- **Records sender, recipient, message content, and timestamp**
- **Tracks message delivery status**
- **Handles database insertion errors**

## Store And Forward Message
- **Main message processing function**
- **Receives messages from clients**
- **Stores messages in database for persistence**
- **Forwards messages to online recipients immediately**
- **Queues messages for offline users**
- **Handles message encryption/decryption coordination**

## Deliver Offline Messages
- **Retrieves and delivers stored messages for newly connected users**
- **Marks delivered messages as read**
- **Sends messages in chronological order**
- **Handles delivery confirmation**

## Deliver Offline Messages To User
- **Public interface for offline message delivery**
- **Called when user comes online**
- **Coordinates with deliver offline messages internal function**

## Get Contacted Users
- **Retrieves list of users that a specific user has communicated with**
- **Queries database for message history**
- **Returns unique list of conversation partners**
- **Sends contacted user list to requesting client**

## Get Chat History
- **Retrieves complete message history between two users**
- **Orders messages chronologically**
- **Sends paginated history to requesting client**
- **Handles large chat histories efficiently**
