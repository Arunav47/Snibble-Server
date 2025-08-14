#include "src/SocketServer.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <dotenv.h>
using namespace std;

SocketServer* server = nullptr;
bool g_verbose = false;

void signalHandler(int signum) {
    if (g_verbose) {
        std::cout << "\n[+] Interrupt signal (" << signum << ") received.\n";
    }
    if (server) {
        server->stop();
    }
    exit(signum);
}

int main() {
    dotenv::init("../.env");
    string host = dotenv::getenv("SOCKET_HOST", "127.0.0.1");
    string port_str = dotenv::getenv("SOCKET_PORT", "8080");
    bool verbose = dotenv::getenv("VERBOSE", "false") == "true";
    g_verbose = verbose;
    cout << "the verbose is " << verbose << endl;
    int port = 8081;
    try {
        port = stoi(port_str);
    } catch (const std::exception& e) {
        cerr << "Invalid SOCKET_PORT value: " << port_str << ", using default 8081" << endl;
        port = 8081;
    }
    string dbPath = dotenv::getenv("DB_PATH", "testdb");
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        if (verbose) {
            cout << host << ":" << port << endl;
        }
        server = new SocketServer(host, port, dbPath);
        
        if (verbose) {
            cout << "[+] Starting Cryptalk Chat Server...\n";
        }
        server->start();
        
        while (true) {
            sleep(1);
        }
        
    } catch (const std::exception& e) {
        cerr << "[-] Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}