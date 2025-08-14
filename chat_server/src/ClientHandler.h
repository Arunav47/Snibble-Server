#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <hiredis/hiredis.h>

class SocketServer;

class ClientHandler {
private:
    bool debugMode = false; // Debug mode flag
    redisContext* redis_context = nullptr; // Redis context for message storage
    redisReply* publisher = nullptr; // Redis publisher for message broadcasting
    void connectToRedis();
    SocketServer* server_ref = nullptr;

public:
    ClientHandler(SocketServer* server);
    ~ClientHandler();
    void clientConnectionHandler();
    void clientDisconnectHandler(const int client_fd);
};

#endif // CLIENT_HANDLER_H