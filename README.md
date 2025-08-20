# Snibble Chat Server

A modern C++ chat server implementation with authentication and secure messaging capabilities. The server consists of two main components: an authentication server and a chat server.

## Architecture Overview

The server is composed of:
- **Auth Server**: Handles user registration, login, and JWT token management
- **Chat Server**: Manages real-time messaging, user presence, and chat functionality
- **Shared Components**: Common database and utility classes

## Prerequisites

### System Requirements
- C++20 compatible compiler (GCC 10+ or Clang 12+)
- CMake 3.16 or higher
- Redis server (for user presence tracking)

### Required Dependencies
- **hiredis**: Redis C client library
- **Crow**: C++ web framework (for auth server)
- **jwt-cpp**: JWT token handling
- **libsodium**: Cryptographic library
- **OpenSSL**: SSL/TLS support
- **laserpants/dotenv**: Environment variable management

## Installation

### 1. Install System Dependencies

#### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libhiredis-dev libssl-dev libsodium-dev
sudo apt install redis-server
```

#### Fedora/RHEL:
```bash
sudo dnf install gcc-c++ cmake pkgconfig
sudo dnf install libhiredis-devel openssl-devel libsodium-devel
sudo dnf install redis
```

### 2. Install Additional Dependencies

Refer to [`../INSTALL_DEPENDENCIES.md`](../INSTALL_DEPENDENCIES.md) for detailed installation instructions for:
- jwt-cpp
- Crow framework
- laserpants/dotenv

### 3. Database Setup

#### Azure SQL Setup:
```bash
# Create Azure SQL server and database
az sql server create --name servername --resource-group your_resource_group --location your_location --admin-user your_admin_user --admin-password your_admin_password
az sql db create --resource-group your_resource_group --server servername --name snibble_users_db --service-objective S0
az sql db create --resource-group your_resource_group --server servername --name snibble_chat_db --service-objective S0

# Configure firewall rules to allow access
az sql server firewall-rule create --resource-group your_resource_group --server servername --name AllowYourIP --start-ip-address your_ip_address --end-ip-address your_ip_address
```

<!-- #### PostgreSQL Setup:
```bash
# Start PostgreSQL service
sudo systemctl start postgresql
sudo systemctl enable postgresql

# Create database user
sudo -u postgres createuser --interactive snibble_db_admin
sudo -u postgres psql -c "ALTER USER snibble_db_admin PASSWORD 'your_password';"

# Create databases
sudo -u postgres createdb snibble_users_db -O snibble_db_admin
sudo -u postgres createdb snibble_chat_db -O snibble_db_admin
``` -->

#### Redis Setup:
```bash
# Start Redis service
sudo systemctl start redis
sudo systemctl enable redis
```

## Configuration

### Environment Variables

Both servers require environment configuration files that are **not tracked in git** for security reasons.

#### Auth Server Configuration
Create `auth_server/.env`:
```bash
AZURE_SQL_SERVER="your_azure_sql_server"
AZURE_SQL_DATABASE="snibble_users_db"
AZURE_SQL_USERNAME="snibble_db_admin"
AZURE_SQL_PASSWORD="your_password"
VERBOSE="false"
PORT=8000
AUTH_HOST="http://127.0.0.1"
AUTH_PORT=8000
SOCKET_HOST="127.0.0.1"
SOCKET_PORT=8080
JWT_SECRET="your_jwt_token"
```

#### Chat Server Configuration
Create `chat_server/.env`:
```bash
SOCKET_HOST="127.0.0.1"
SOCKET_PORT=8080
AZURE_SQL_SERVER="your_azure_sql_server"
AZURE_SQL_DATABASE="snibble_chat_db"
AZURE_SQL_USERNAME="snibble_db_admin"
AZURE_SQL_PASSWORD="your_password"
VERBOSE="false"
```

### Security Notes
- **Never commit `.env` files** - they contain sensitive credentials
- Change default passwords and JWT secrets in production
- Use strong, unique passwords for database access
- Consider using environment-specific configurations

## Building

### Build Auth Server
```bash
cd auth_server
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Build Chat Server
```bash
cd chat_server
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

The executables will be created in the respective `build/bin/` directories (as ignored by `.gitignore`).

## Running the Servers

### 1. Start Auth Server
```bash
cd auth_server/build/bin
./SnibbleAuthServer
```
The auth server will start on `http://localhost:8000`

### 2. Start Chat Server
```bash
cd chat_server/build/bin
./SnibbleChatServer
```
The chat server will start on `localhost:8080`

## Development

### Project Structure
```
server/
├── auth_server/          # Authentication server
│   ├── src/             # Source files
│   ├── CMakeLists.txt   # Build configuration
│   ├── main.cpp         # Entry point
│   └── .env             # Environment config (not in git)
├── chat_server/         # Chat server
│   ├── src/             # Source files
│   ├── CMakeLists.txt   # Build configuration
│   ├── main.cpp         # Entry point
│   └── .env             # Environment config (not in git)
└── shared/              # Shared components
    ├── include/         # Header files
    └── src/             # Implementation files
```

### Ignored Files (.gitignore)
The following files/directories are not tracked in git:
- `*/build/` - Build directories and compiled binaries
- `*.o`, `*.so`, `*.exe` - Compiled object files and executables
- `*.env` - Environment configuration files
- Debug and intermediate build files

### Adding New Features
1. Shared functionality goes in `shared/`
2. Authentication features go in `auth_server/src/`
3. Chat features go in `chat_server/src/`
4. Update respective CMakeLists.txt files when adding new source files

## API Endpoints

### Auth Server (Port 8000)
- `POST /login` - User authentication, returns JWT token
- `POST /signup` - User registration
- `POST /logout` - User logout (clears client-side session)
- `POST /verify-token` - JWT token verification (for silent login)
  - Requires `Authorization: Bearer <token>` header
  - Returns user info if token is valid
- `GET /search?q=<query>` - Search for users by username
- `POST /store_public_key` - Store user's public key for encryption
- `POST /get_public_key` - Retrieve user's public key

### JWT Security Model
**Important Security Note**: JWT verification happens **exclusively on the server side**:

1. **Token Generation**: Server creates signed JWT with secret key
2. **Token Storage**: Client stores token securely (no access to secret)
3. **Token Usage**: Client sends token in `Authorization: Bearer <token>` header
4. **Token Verification**: Server verifies signature and expiration server-side
5. **Silent Login**: Client uses `/verify-token` endpoint to validate stored tokens

**The client never has access to the JWT secret key - this ensures proper security.**

### Chat Server (Port 8080)
- WebSocket connection for real-time messaging
- Message broadcasting and private messaging
- User presence tracking

## Testing

Tests are available in the `../tests/` directory. Refer to the main project documentation for testing instructions.

## Troubleshooting

### Common Issues

1. **Database Connection Failed**
   - Check PostgreSQL service is running
   - Verify database credentials in `.env` files
   - Ensure databases exist and user has proper permissions

2. **Redis Connection Failed**
   - Check Redis service is running: `sudo systemctl status redis`
   - Verify Redis is accessible on default port 6379

3. **Build Errors**
   - Ensure all dependencies are installed
   - Check CMake version compatibility
   - Verify C++20 compiler support

4. **Port Already in Use**
   - Check if another instance is running
   - Use `netstat -tlnp | grep :8000` to check port usage
   - Modify port numbers in `.env` files if needed

### Debug Mode
Set `VERBOSE="true"` in `.env` files to enable detailed logging.

## Security Considerations

### JWT Security (CRITICAL)
- **JWT secrets live ONLY on the server** - never expose to client applications
- **Token verification happens exclusively server-side** via `/verify-token` endpoint
- **Clients only store and transmit tokens** - they never decrypt or verify JWT signatures
- **Silent login** implemented through server-side token validation, not client-side verification

### General Security Best Practices
- Never commit `.env` files or credentials to version control
- Use strong, unique passwords for all services (minimum 16 characters)
- Regularly rotate JWT secrets (recommended: monthly in production)
- Enable SSL/TLS in production environments
- Implement proper firewall rules and network segmentation
- Keep dependencies updated and scan for vulnerabilities
- Use HTTPS for all client-server communication in production
- Implement rate limiting on authentication endpoints
- Consider implementing refresh tokens for long-lived sessions

### Production Deployment
- Use environment variables instead of `.env` files in production
- Implement proper logging and monitoring
- Use a reverse proxy (nginx/Apache) with SSL termination
- Deploy databases with proper access controls and encryption
- Regular security audits and penetration testing

## Contributing

1. Follow the existing code style and structure
2. Add tests for new features
3. Update documentation as needed
4. Ensure `.env` files are not committed
5. Test both servers together before submitting changes

