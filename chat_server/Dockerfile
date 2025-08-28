#=============================Build Stage===================================
FROM alpine:3.20 as builder

RUN apk add --no-cache \
    boost-dev \
    g++ \
    hiredis \
    unixodbc \
    build-base \
    cmake \
    make

WORKDIR /
COPY /shared /shared
WORKDIR /src
COPY auth_server/CMakeLists.txt .
COPY auth_server/main.cpp .
COPY auth_server/src/ src/
