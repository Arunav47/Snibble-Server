#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <pthread.h>
#include <hiredis/hiredis.h>

class ClientHandler;
class MessageHandler;

class SocketServer {
    friend class ClientHandler;
    friend class MessageHandler;
private:
    bool debugMode = false;
    std::string HOST;
    int PORT;
    int MAX_CLIENTS = 100;
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    std::map<int, pthread_t> client_threads;
    pthread_t accept_client_thread, read_msg_thread, sub_thread;
    std::vector<int> client_fds;
    std::map<std::string, int> client_map;
    std::map<int, std::string> client_names;
    std::map<std::string, bool> isOnline;
    pthread_mutex_t mutex;
    volatile bool running = true;
    std::string dbPath;
public:
    SocketServer(const std::string& host, const int port, const std::string& dbPath);
    ~SocketServer();

    void start();
    void stop();
};

#endif