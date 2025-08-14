#include "SocketServer.h"
#include "ClientHandler.h"
#include "MessageHandler.h"

using namespace std;


SocketServer::SocketServer(const std::string& host, int port, const std::string& dbPath) 
    : HOST(host), PORT(port), dbPath(dbPath), server_fd(-1) {
    pthread_mutex_init(&mutex, nullptr);
}

SocketServer::~SocketServer() {
    if(running) {
        stop();
    }
}

void SocketServer::start() {
    try {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd < 0) {
            stop();
            throw "Socket formation unsuccessful";
        }
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = inet_addr(HOST.c_str());
        if(bind(server_fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0) {
            stop();
            throw "Binding socket unsuccessful";
        }
        if(listen(server_fd, 100) < 0) {
            stop();
            throw "Listening on socket unsuccessful";
        }
    } catch(const char* msg) {
        stop();
        if (debugMode) {
            cerr << "Error: " << msg << endl;
        }
        return;
    } catch(...) {
        stop();
        if (debugMode) {
            cerr << "An unexpected error occurred while starting the server." << endl;
        }
        return;
    }
    if (debugMode) {
        cout << "[+] Server initialized successfully on " << HOST << ":" << PORT << endl;
    }
    running = true;
    pthread_create(&accept_client_thread, nullptr, [](void* arg)->void* {
        SocketServer* server = static_cast<SocketServer*>(arg);
        ClientHandler clientHandler(server);
        clientHandler.clientConnectionHandler();
        return nullptr;
    }, this);
    if (debugMode) {
        printf("[+] Accepting client connections...\n");
    }
}

void SocketServer::stop() {
    running = false;
    shutdown(server_fd, SHUT_RDWR);
    pthread_mutex_lock(&mutex);
    for (int fd : client_fds) {
        close(fd);
    }
    client_fds.clear();
    pthread_mutex_unlock(&mutex);
    if (debugMode) {
        printf("[+] Stopping server...\n");
    }
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    
    pthread_join(accept_client_thread, nullptr);
    
    for (auto& entry : client_threads) {
        pthread_join(entry.second, nullptr);
    }
    
    pthread_mutex_destroy(&mutex);
    if (debugMode) {
        printf("[+] Server shut down cleanly\n");
    }
}