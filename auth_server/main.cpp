#include "crow.h"
#include "src/AuthManager.h"
#include <dotenv.h>
#include <iostream>
#include <string>
#include <jwt-cpp/jwt.h>
#include <chrono>

using namespace std;

// JWT secret key (in production, store this securely in environment variables)

// Helper function to create JWT token
string createJWTToken(const string& username, const string& JWT_SECRET) {
    auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
        .set_issuer("snibble-auth")
        .set_type("JWT")
        .set_payload_claim("username", jwt::claim(username))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours{120}) // 120 hour expiry
        .sign(jwt::algorithm::hs256{JWT_SECRET});
    
    return token;
}

int main() {
    dotenv::init("../.env");
    string dbPath = dotenv::getenv("DB_PATH", "testdb");
    bool verbose = dotenv::getenv("VERBOSE") == "true";
    int port = stoi(dotenv::getenv("PORT", "8000"));
    string JWT_SECRET = dotenv::getenv("JWT_SECRET", "your-super-secret-key-change-this-in-production");

    crow::SimpleApp app;
    AuthManager authManager(verbose, dbPath);

    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([&authManager, &JWT_SECRET](const crow::request& req) {
        auto json = crow::json::load(req.body);
        if (!json) {
            return crow::response(400, "Invalid JSON");
        }

        string username = json["username"].s();
        string password = json["password"].s();
        
        if (authManager.isUserRegistered(username)) {
            if (authManager.login(username, password)) {

                string token = createJWTToken(username, JWT_SECRET);
                
                crow::json::wvalue response;
                response["message"] = "Login successful";
                response["token"] = token;
                response["username"] = username;
                
                return crow::response(200, response);
            } else {
                return crow::response(401, "Invalid credentials");
            } 
        } else {
            return crow::response(404, "Invalid credentials");
        }
    });

    CROW_ROUTE(app, "/signup").methods(crow::HTTPMethod::POST)([&authManager](const crow::request& req) {
        auto json = crow::json::load(req.body);
        if(!json) {
            return crow::response(400, "Invalid JSON");
        }
        string username = json["username"].s();
        string password = json["password"].s();
        if (authManager.isUserRegistered(username)) {
            return crow::response(401, "User Already Exist");
        } else {
            if (authManager.signup(username, password)) {
                return crow::response(201, "User Created Successfully");
            } else {
                return crow::response(500, "Internal Server Error");
            }
        }
    });

    CROW_ROUTE(app, "/search").methods(crow::HTTPMethod::GET)([&authManager](const crow::request& req) {
        auto query = req.url_params.get("q");
        if (!query) {
            return crow::response(400, "Missing search query parameter 'q'");
        }
        
        string searchTerm = string(query);
        if (searchTerm.length() < 2) {
            return crow::response(400, "Search query must be at least 2 characters");
        }
        
        try {
            vector<string> results = authManager.searchUsers(searchTerm);
            
            // Create JSON array response
            crow::json::wvalue response;
            response = crow::json::wvalue::list();
            
            for (size_t i = 0; i < results.size(); ++i) {
                response[i] = results[i];
            }
            
            return crow::response(200, response);
        } catch (const exception& e) {
            return crow::response(500, "Search failed: " + string(e.what()));
        }
    });

    // Store public key endpoint
    CROW_ROUTE(app, "/store_public_key").methods(crow::HTTPMethod::POST)([&authManager](const crow::request& req) {
        auto json = crow::json::load(req.body);
        if (!json) {
            return crow::response(400, "Invalid JSON");
        }

        if (!json.has("username") || !json.has("public_key")) {
            return crow::response(400, "Missing username or public_key");
        }

        string username = json["username"].s();
        string publicKey = json["public_key"].s();

        if (authManager.storePublicKey(username, publicKey)) {
            return crow::response(200, "Public key stored successfully");
        } else {
            return crow::response(500, "Failed to store public key");
        }
    });


    CROW_ROUTE(app, "/get_public_key").methods(crow::HTTPMethod::POST)([&authManager](const crow::request& req) {
        auto json = crow::json::load(req.body);
        if (!json) {
            return crow::response(400, "Invalid JSON");
        }

        if (!json.has("recipient")) {
            return crow::response(400, "Missing recipient");
        }

        string recipient = json["recipient"].s();
        string publicKey = authManager.getPublicKey(recipient);

        if (!publicKey.empty()) {
            return crow::response(200, publicKey);
        } else {
            return crow::response(404, "Public key not found for user");
        }
    });

    CROW_ROUTE(app, "/verify-token").methods(crow::HTTPMethod::POST)([&JWT_SECRET](const crow::request& req) {
        string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty()) {
            return crow::response(401, "Missing Authorization header");
        }

        const string bearerPrefix = "Bearer ";
        if (authHeader.find(bearerPrefix) != 0) {
            return crow::response(401, "Invalid Authorization header format");
        }

        string token = authHeader.substr(bearerPrefix.length());
        if (token.empty()) {
            return crow::response(401, "Missing token");
        }

        try {
            auto decoded = jwt::decode(token);

            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET})
                .with_type("JWT")
                .with_issuer("snibble-auth")
                .leeway(60UL);

            verifier.verify(decoded);

            auto now = std::chrono::system_clock::now();
            auto exp = decoded.get_expires_at();
            if (exp < now) {
                return crow::response(401, "Token has expired");
            }

            string username;
            if (decoded.has_payload_claim("username")) {
                username = decoded.get_payload_claim("username").as_string();
            } else {
                return crow::response(401, "Invalid token payload");
            }

            crow::json::wvalue response;
            response["valid"] = true;
            response["username"] = username;
            response["message"] = "Token is valid";

            return crow::response(200, response);

        } catch (const jwt::error::signature_verification_exception& e) {
            return crow::response(401, "Token signature verification failed");
        } catch (const jwt::error::token_verification_exception& e) {
            return crow::response(401, "Token verification failed");
        } catch (const std::exception& e) {
            return crow::response(401, "Token verification error: " + string(e.what()));
        }
    });

    // Logout endpoint (optional - can be used to invalidate sessions)
    CROW_ROUTE(app, "/logout").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        // In a stateless JWT system, logout is typically handled client-side
        // by removing the token. However, for better security, you could maintain
        // a blacklist of tokens or use refresh tokens.
        
        crow::json::wvalue response;
        response["message"] = "Logout successful";
        return crow::response(200, response);
    });

    app.port(port).multithreaded().bindaddr("127.0.0.1").run();

    return 0;
}
