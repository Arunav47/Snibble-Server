#include "SocketServer.h"
#include "ClientHandler.h"
#include "MessageHandler.h"

using namespace std;


SocketServer::SocketServer(const string& host, int port, const string& server, const string& database, const string& username, const string& password) 
    : HOST(host), PORT(port), SERVER(server), DATABASE(database), USERNAME(username), PASSWORD(password), server_fd(-1) {
    pthread_mutex_init(&mutex, nullptr);
    redis_context = redisConnect("127.0.0.1", 6379);
    if (redis_context == nullptr || redis_context->err) {
        if (redis_context) {
            if (debugMode) {
                cerr << "Redis connection error: " << redis_context->errstr << endl;
            }
            redisFree(redis_context);
        } else {
            if (debugMode) {
                cerr << "Redis connection error: Can't allocate redis context" << endl;
            }
        }
        redis_context = nullptr;
    } else if (debugMode) {
        cout << "[+] Connected to Redis server successfully" << endl;
    }
}

SocketServer::~SocketServer() {
    if(running) {
        stop();
    }
    if (redis_context) {
        redisFree(redis_context);
        redis_context = nullptr;
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

void SocketServer::sendOnlineUsersList(int client_fd) {
    pthread_mutex_lock(&mutex);
    std::string online_users = "ONLINE_USERS:";
    for (const auto& pair : isOnline) {
        if (pair.second) {
            online_users += pair.first + ",";
        }
    }
    if (online_users.back() == ',') {
        online_users.pop_back();
    }
    send(client_fd, online_users.c_str(), online_users.length(), 0);
    pthread_mutex_unlock(&mutex);
}

void SocketServer::broadcastUserStatus(const std::string& username, bool online) {
    pthread_mutex_lock(&mutex);
    isOnline[username] = online;
    
    if (redis_context) {
        // Publish the status change to Redis
        // Channel name is the username, message is either "joined" or "left"
        redisReply* reply = (redisReply*)redisCommand(redis_context, 
            "PUBLISH %s %s", 
            username.c_str(), 
            online ? "joined" : "left"
        );
        
        if (reply) {
            if (debugMode && reply->type == REDIS_REPLY_ERROR) {
                cerr << "Redis publish error: " << reply->str << endl;
            }
            freeReplyObject(reply);
        }
        
        // Also maintain a Redis set of online users
        if (online) {
            reply = (redisReply*)redisCommand(redis_context, 
                "SADD online_users %s", 
                username.c_str()
            );
        } else {
            reply = (redisReply*)redisCommand(redis_context, 
                "SREM online_users %s", 
                username.c_str()
            );
        }
        
        if (reply) {
            freeReplyObject(reply);
        }
    }
    
    pthread_mutex_unlock(&mutex);
}
