#include "MessageHandler.h"
#include "SocketServer.h"
#include "ClientHandler.h"
#include <chrono>
using namespace std;

MessageHandler::MessageHandler(SocketServer* server, ClientHandler* handler, const string& dbPath) 
    : server_ref(server), client_handler(handler) {
    connectToDatabase(dbPath);
}

MessageHandler::~MessageHandler() {
}

void MessageHandler::storeAndForwardMessage(const int client_fd) {
    char buffer[1024];
    while (server_ref->running) {
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0 || !server_ref->running) {
            if (bytes_received < 0) {
                cerr << "[-] Error receiving message: " << strerror(errno) << endl;
            } else {
                cout << "[+] Client disconnected normally." << endl;
            }
            if (client_handler) {
                client_handler->clientDisconnectHandler(client_fd);
            }
            break;
        }
        buffer[bytes_received] = '\0';
        string message(buffer);
        
        if (message.substr(0, 16) == "GET_CONTACTS_FOR") {
            size_t pos = message.find(':');
            if (pos != string::npos) {
                string requested_username = message.substr(pos + 1);
                requested_username.erase(requested_username.find_last_not_of(" \n\r\t") + 1);
                getContactedUsers(requested_username, client_fd);
            }
            continue;
        }
        
        if (message.substr(0, 17) == "GET_CHAT_HISTORY:") {
            // Format: GET_CHAT_HISTORY:username:otheruser
            size_t pos1 = message.find(':', 17);
            if (pos1 != string::npos) {
                string username = message.substr(17, pos1 - 17);
                string otherUser = message.substr(pos1 + 1);
                // Remove any trailing newline or whitespace
                username.erase(username.find_last_not_of(" \n\r\t") + 1);
                otherUser.erase(otherUser.find_last_not_of(" \n\r\t") + 1);
                getChatHistory(username, otherUser, client_fd);
            }
            continue;
        }
        
        size_t pos1 = message.find(':');
        size_t pos2 = message.find(':', pos1 + 1);
        if (pos1 == string::npos || pos2 == string::npos) continue;
    
        string sender = message.substr(0, pos1);
        string recipient = message.substr(pos1 + 1, pos2 - pos1 - 1);
        string msg_content = message.substr(pos2 + 1);

        pthread_mutex_lock(&server_ref->mutex);
        if (server_ref->client_map.count(recipient)) {
            // Recipient is online - send message immediately
            int dest_fd = server_ref->client_map[recipient];
            string msg_to_send = sender + ": " + msg_content;
            send(dest_fd, msg_to_send.c_str(), msg_to_send.length(), 0);
            
            // Store message in database as delivered (for message history)
            storeMessageInDatabase(sender, recipient, msg_content, true);
        } else {
            // Recipient is offline - store message in database for later delivery
            storeMessageInDatabase(sender, recipient, msg_content, false);
            string success_msg = "Server: Message stored for offline user '" + recipient + "'.\n";
            send(client_fd, success_msg.c_str(), success_msg.length(), 0);
        }
        pthread_mutex_unlock(&server_ref->mutex);
    }
    
}

void MessageHandler::connectToDatabase(const string& dbPath) {
    db_manager = DatabaseManager::getInstance(dbPath, true);
    if (!db_manager || !db_manager->isConnected()) {
        cout << "Failed to connect to database for message storage" << endl;
    } else {
        cout << "Connected to database for message storage" << endl;
    }
}

void MessageHandler::storeMessageInDatabase(const string& sender, const string& recipient, const string& message, bool delivered) {
    if (!db_manager || !db_manager->isConnected()) {
        cout << "Database not connected, cannot store message" << endl;
        return;
    }
    
    try {
        // Create a unique conversation ID (consistent ordering)
        string conversation_id = (sender < recipient) ? sender + ":" + recipient : recipient + ":" + sender;
        
        // Store message in database with delivered flag
        vector<string> params = {sender, recipient, message, conversation_id, delivered ? "true" : "false"};
        bool success = db_manager->executeParamUpdate(
            "INSERT INTO messages (sender, recipient, message_content, conversation_id, delivered) VALUES ($1, $2, $3, $4, $5)", 
            params
        );
        
        if (success) {
            cout << "Message stored in database for conversation: " << conversation_id << " (delivered: " << delivered << ")" << endl;
        } else {
            cout << "Failed to store message in database" << endl;
        }
    }
    catch (const exception& e) {
        cout << "Error storing message in database: " << e.what() << endl;
    }
}

void MessageHandler::deliverOfflineMessages(const string& username, int client_fd) {
    if (!db_manager || !db_manager->isConnected()) {
        cout << "Database not connected, cannot retrieve offline messages" << endl;
        return;
    }
    
    try {
        // Retrieve all undelivered messages for this user
        vector<string> params = {username};
        pqxx::result result = db_manager->executeParamQuery(
            "SELECT sender, message_content, timestamp FROM messages WHERE recipient = $1 AND delivered = false ORDER BY timestamp ASC", 
            params
        );
        
        if (result.size() > 0) {
            string offline_notification = "Server: You have " + to_string(result.size()) + " offline message(s):\n";
            send(client_fd, offline_notification.c_str(), offline_notification.length(), 0);
            
            // Send each offline message
            for (const auto& row : result) {
                string sender = row["sender"].c_str();
                string message_content = row["message_content"].c_str();
                string timestamp = row["timestamp"].c_str();
                
                string offline_msg = "[OFFLINE] " + sender + " (" + timestamp + "): " + message_content + "\n";
                send(client_fd, offline_msg.c_str(), offline_msg.length(), 0);
            }
            
            // Mark messages as delivered
            bool success = db_manager->executeParamUpdate(
                "UPDATE messages SET delivered = true WHERE recipient = $1 AND delivered = false", 
                params
            );
            
            if (success) {
                cout << "Marked " << result.size() << " offline messages as delivered for user: " << username << endl;
            }
        }
    }
    catch (const exception& e) {
        cout << "Error retrieving offline messages: " << e.what() << endl;
    }
}

void MessageHandler::deliverOfflineMessagesToUser(const string& username, int client_fd) {
    deliverOfflineMessages(username, client_fd);
}

void MessageHandler::getContactedUsers(const string& username, int client_fd) {
    if (!db_manager || !db_manager->isConnected()) {
        cout << "Database not connected, cannot retrieve contacted users" << endl;
        string error_msg = "Server: Error retrieving contacted users\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
        return;
    }
    
    try {
        vector<string> params = {username, username};
        pqxx::result result = db_manager->executeParamQuery(
            "SELECT DISTINCT CASE "
            "WHEN sender = $1 THEN recipient "
            "WHEN recipient = $2 THEN sender "
            "END as contacted_user "
            "FROM messages "
            "WHERE (sender = $1 OR recipient = $2) "
            "AND CASE "
            "WHEN sender = $1 THEN recipient "
            "WHEN recipient = $2 THEN sender "
            "END IS NOT NULL "
            "ORDER BY contacted_user", 
            params
        );
        
        if (result.size() > 0) {
            string contacted_list = "CONTACTED_USERS:";
            for (const auto& row : result) {
                string contacted_user = row["contacted_user"].c_str();
                if (!contacted_user.empty()) {
                    contacted_list += contacted_user + ",";
                }
            }
            // Remove trailing comma
            if (contacted_list.back() == ',') {
                contacted_list.pop_back();
            }
            contacted_list += "\n";
            
            send(client_fd, contacted_list.c_str(), contacted_list.length(), 0);
            cout << "Sent contacted users list to " << username << ": " << result.size() << " users" << endl;
        } else {
            string no_contacts = "CONTACTED_USERS:\n";
            send(client_fd, no_contacts.c_str(), no_contacts.length(), 0);
            cout << "No contacted users found for " << username << endl;
        }
    }
    catch (const exception& e) {
        cout << "Error retrieving contacted users: " << e.what() << endl;
        string error_msg = "Server: Error retrieving contacted users\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
    }
}

void MessageHandler::getChatHistory(const string& username, const string& otherUser, int client_fd) {
    if (!db_manager || !db_manager->isConnected()) {
        cout << "Database not connected, cannot retrieve chat history" << endl;
        string error_msg = "CHAT_HISTORY_ERROR:Database not connected\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
        return;
    }
    
    try {
        // Get all messages between these two users, ordered by timestamp
        vector<string> params = {username, otherUser, otherUser, username};
        pqxx::result result = db_manager->executeParamQuery(
            "SELECT sender, recipient, message_content, timestamp, delivered "
            "FROM messages "
            "WHERE (sender = $1 AND recipient = $2) OR (sender = $3 AND recipient = $4) "
            "ORDER BY timestamp ASC", 
            params
        );
        
        if (result.size() > 0) {
            string history_response = "CHAT_HISTORY_START:" + username + ":" + otherUser + "\n";
            send(client_fd, history_response.c_str(), history_response.length(), 0);
            
            // Send each message in the conversation
            for (const auto& row : result) {
                string sender = row["sender"].c_str();
                string recipient = row["recipient"].c_str();
                string message_content = row["message_content"].c_str();
                string timestamp = row["timestamp"].c_str();
                bool delivered = row["delivered"].as<bool>();
                
                // Format: CHAT_HISTORY_MSG:sender:recipient:message:timestamp:delivered
                string msg_line = "CHAT_HISTORY_MSG:" + sender + ":" + recipient + ":" + 
                                 message_content + ":" + timestamp + ":" + (delivered ? "true" : "false") + "\n";
                send(client_fd, msg_line.c_str(), msg_line.length(), 0);
            }
            
            string history_end = "CHAT_HISTORY_END:" + username + ":" + otherUser + "\n";
            send(client_fd, history_end.c_str(), history_end.length(), 0);
            
            cout << "Sent chat history to " << username << " for conversation with " << otherUser 
                 << ": " << result.size() << " messages" << endl;
        } else {
            string no_history = "CHAT_HISTORY_START:" + username + ":" + otherUser + "\n";
            send(client_fd, no_history.c_str(), no_history.length(), 0);
            string history_end = "CHAT_HISTORY_END:" + username + ":" + otherUser + "\n";
            send(client_fd, history_end.c_str(), history_end.length(), 0);
            cout << "No chat history found between " << username << " and " << otherUser << endl;
        }
    }
    catch (const exception& e) {
        cout << "Error retrieving chat history: " << e.what() << endl;
        string error_msg = "CHAT_HISTORY_ERROR:Error retrieving chat history\n";
        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
    }
}


