#include "ClientHandler.h"
#include "SocketServer.h"
#include "MessageHandler.h"
using namespace std;


struct ThreadArgs {
    SocketServer* server;
    ClientHandler* client_handler;
    int client_fd;
};

ClientHandler::ClientHandler(SocketServer* server) : server_ref(server) {
    connectToRedis();
}

ClientHandler::~ClientHandler() {
    if (redis_context) {
        redisFree(redis_context);
    }
    if (debugMode) {
        cout << "[+] ClientHandler destroyed." << endl;
    }
    if (publisher) {
        freeReplyObject(publisher);
    }
}

void ClientHandler::clientConnectionHandler() {
    
    socklen_t addr_len = sizeof(server_ref->client_addr);
    while(server_ref->running) {
        int client_fd = accept(server_ref->server_fd, (struct sockaddr*)& server_ref->client_addr, &addr_len);
        if(client_fd < 0) {
            if(!server_ref->running) break;
            if (debugMode) {
                cerr << "[-] Error accepting client connection: " << strerror(errno) << endl;
            }
            continue;
        }
        char client_name[100];
        int bytes_receiver = recv(client_fd, client_name, sizeof(client_name) - 1, 0);
        if (bytes_receiver <= 0) {
            if (debugMode) {
                cerr << "[-] Error receiving client name: " << strerror(errno) << endl;
            }
            close(client_fd);
            continue;
        }
        client_name[bytes_receiver] = '\0';
        pthread_mutex_lock(&server_ref->mutex);
        string name(client_name);
        server_ref->client_fds.push_back(client_fd);
        server_ref->client_map[name] = client_fd;
        server_ref->client_names[client_fd] = name;
        
        MessageHandler* offlineMessageHandler = new MessageHandler(server_ref, this, server_ref->dbPath);
        offlineMessageHandler->deliverOfflineMessagesToUser(name, client_fd);
        delete offlineMessageHandler;
        
        pthread_t thread_id;
        ThreadArgs* args = new ThreadArgs{server_ref, this, client_fd};
        pthread_create(&thread_id, nullptr, [](void* arg)->void* {
            ThreadArgs* args = static_cast<ThreadArgs*>(arg);
            MessageHandler* msgHandler = new MessageHandler(args->server, args->client_handler, args->server->dbPath);
            msgHandler->storeAndForwardMessage(args->client_fd);
            delete msgHandler;
            delete args;
            return nullptr;
        }, args);
        pthread_detach(thread_id);
        server_ref->client_threads[client_fd] = thread_id;
        publisher = (redisReply*)redisCommand(redis_context, "PUBLISH %s %s", name.c_str(), "joined");
        if (publisher) {
            freeReplyObject(publisher);
            publisher = nullptr;
        }
        pthread_mutex_unlock(&server_ref->mutex);
    }
}

void ClientHandler::clientDisconnectHandler(const int client_fd) {
    pthread_mutex_lock(&server_ref->mutex);
    
    auto it = find(server_ref->client_fds.begin(), server_ref->client_fds.end(), client_fd);
    if (it != server_ref->client_fds.end()) {
        server_ref->client_fds.erase(it);
        
        string name = server_ref->client_names[client_fd];
        server_ref->client_map.erase(name);
        server_ref->client_names.erase(client_fd);
        server_ref->client_threads.erase(client_fd);
        if (redis_context) {
            publisher = (redisReply*)redisCommand(redis_context, "PUBLISH %s %s", name.c_str(), "left");
            if (publisher) {
                freeReplyObject(publisher);
                publisher = nullptr;
            }
        }
        close(client_fd);
        if (debugMode) {
            cout << "[+] Client " << name << " disconnected." << endl;
        }
    }
    
    pthread_mutex_unlock(&server_ref->mutex);
}


void ClientHandler::connectToRedis() {
    redis_context = redisConnect("127.0.0.1", 6379);
    if (redis_context == nullptr || redis_context->err) {
        if (redis_context) {
            if (debugMode) {
                cerr << "[-] Error connecting to Redis: " << redis_context->errstr << endl;
            }
            redisFree(redis_context);
        } else {
            if (debugMode) {
                cerr << "[-] Can't allocate redis context" << endl;
            }
        }
    }
    else {
        if (debugMode) {
            cout << "[+] Connected to Redis successfully." << endl;
        }
    }
}