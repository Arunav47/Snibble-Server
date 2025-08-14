cmake_minimum_required(VERSION 3.16)

project(SnibbleAuthServer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Find libpqxx (PostgreSQL C++ library)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Find crow framework
find_package(Crow REQUIRED)

# Find sodium (crypto library)
pkg_check_modules(SODIUM REQUIRED libsodium)

# Find dotenv library
find_package(laserpants_dotenv REQUIRED)

find_path(JWT_CPP_INCLUDE_DIR jwt-cpp/jwt.h)
if(NOT JWT_CPP_INCLUDE_DIR)
    message(FATAL_ERROR "jwt-cpp not found. Please install it via vcpkg or manually.")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../shared/include)
include_directories(${SODIUM_INCLUDE_DIRS})
include_directories(${PQXX_INCLUDE_DIRS})
include_directories(${JWT_CPP_INCLUDE_DIR})
include_directories(/usr/local/include/laserpants/dotenv-0.9.3)

# Define source files
set(SOURCES
    main.cpp
    src/AuthManager.cpp
    src/EncryptionManager.cpp
    ../shared/src/DatabaseManager.cpp
)

# Define header files
set(HEADERS
    src/AuthManager.h
    src/EncryptionManager.h
    ../shared/include/DatabaseManager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    ${PQXX_LIBRARIES}
    Crow::Crow
    ${SODIUM_LIBRARIES}
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE ${SODIUM_CFLAGS} ${PQXX_CFLAGS} -Wno-deprecated-declarations)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)