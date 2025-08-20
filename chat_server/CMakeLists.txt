cmake_minimum_required(VERSION 3.16)

project(SnibbleChatServer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(laserpants_dotenv REQUIRED)

# Find ODBC for SQL Server connectivity
find_package(ODBC REQUIRED)

# Find hiredis (still needed for user presence tracking)
pkg_check_modules(HIREDIS REQUIRED hiredis)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../shared/include)
include_directories(/usr/local/include/laserpants/dotenv-0.9.3)
include_directories(${HIREDIS_INCLUDE_DIRS})
include_directories(${ODBC_INCLUDE_DIRS})

set(SOURCES
    main.cpp
    src/SocketServer.cpp
    src/ClientHandler.cpp
    src/MessageHandler.cpp
    ../shared/src/DatabaseManager.cpp
)

set(HEADERS
    src/SocketServer.h
    src/ClientHandler.h
    src/MessageHandler.h
    ../shared/include/DatabaseManager.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    Threads::Threads
    ${HIREDIS_LIBRARIES}
    ${ODBC_LIBRARIES}
)

target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall -Wextra -pedantic -pthread -Wno-deprecated-declarations
    ${HIREDIS_CFLAGS_OTHER}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)