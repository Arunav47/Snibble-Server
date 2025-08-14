#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H

#include <string>
#include <sodium.h>

class EncryptionManager {
    static std::string SECRET_KEY;
    static const size_t SALT_SIZE = crypto_pwhash_SALTBYTES;
    static const size_t HASH_SIZE = crypto_pwhash_STRBYTES;

public:
    EncryptionManager();
    ~EncryptionManager();
    static std::string hashPassword(const std::string& password);
    static bool verifyPassword(const std::string& password, const std::string& hashedPassword);
};

#endif
