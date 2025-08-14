# ENCRYPTION_MANAGER

**This documentation is for the functions of the EncryptionManager class if ever needed to change in future**

- Handles password hashing and verification using libsodium
- Provides secure cryptographic operations
- Static class with no instance variables

## Constructor
- **Initializes libsodium crypto library**
- **Sets up cryptographic parameters**
- **No instance-specific data**

## Destructor
- **Cleans up any crypto resources if needed**
- **Currently minimal cleanup required**

## Hash Password
- **Takes a plain text password as input**
- **Uses libsodium's crypto_pwhash for secure hashing**
- **Applies salt and configurable work factors**
- **Returns base64-encoded hash string**
- **Uses crypto_pwhash_OPSLIMIT_SENSITIVE and crypto_pwhash_MEMLIMIT_SENSITIVE for security**

## Verify Password
- **Takes plain password and hashed password as input**
- **Uses libsodium's crypto_pwhash_str_verify for verification**
- **Constant-time comparison to prevent timing attacks**
- **Returns true if password matches hash, false otherwise**
- **Handles verification errors and invalid hash formats**
