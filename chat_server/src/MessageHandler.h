#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "DatabaseManager.h"

class SocketServer;
class ClientHandler;

class MessageHandler {
private:
    SocketServer* server_ref = nullptr;
    ClientHandler* client_handler = nullptr;
    DatabaseManager* db_manager = nullptr;
    
    void connectToDatabase(const std::string& dbPath);
    void storeMessageInDatabase(const std::string& sender, const std::string& recipient, const std::string& message, bool delivered = true);
    void deliverOfflineMessages(const std::string& username, int client_fd);
    void getContactedUsers(const std::string& username, int client_fd);
    void getChatHistory(const std::string& username, const std::string& otherUser, int client_fd);

public:
    MessageHandler(SocketServer* server, ClientHandler* handler = nullptr, const std::string& dbPath = "");
    ~MessageHandler();
    void storeAndForwardMessage(const int client_fd);
    void deliverOfflineMessagesToUser(const std::string& username, int client_fd);
};

#endif