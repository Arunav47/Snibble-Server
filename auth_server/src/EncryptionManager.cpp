#include "EncryptionManager.h"
#include <sodium.h>
#include <dotenv.h>
using namespace std;

std::string EncryptionManager::SECRET_KEY = "";

EncryptionManager::EncryptionManager() {
    dotenv::init("server/auth_server/.env");
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }
    SECRET_KEY = dotenv::getenv("SECRET_KEY", "default_secret_key");
}


string EncryptionManager::hashPassword(const std::string& password) {
    std::string input = SECRET_KEY + password;
    char hash[crypto_pwhash_STRBYTES];
    try {
        if (crypto_pwhash_str(
                hash,
                input.c_str(), input.size(),
                crypto_pwhash_OPSLIMIT_MODERATE,
                crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
            throw std::runtime_error("Hashing failed (out of memory?)");
        }
        return string(hash);
    } catch (const std::exception& e) {
        throw std::runtime_error("Hashing failed: " + std::string(e.what()));
        return "";
    }

}

bool EncryptionManager::verifyPassword(const std::string& password, const std::string& storedHash) {
    std::string input = SECRET_KEY + password;
    try {
        if (crypto_pwhash_str_verify(
                storedHash.c_str(),
                input.c_str(), input.size()) != 0) {
            return false;
        }
        return true;

    } catch (const std::exception& e) {
        throw std::runtime_error("Verification failed: " + std::string(e.what()));
        return false;
    }
}