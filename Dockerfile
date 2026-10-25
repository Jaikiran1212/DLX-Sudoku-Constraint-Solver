
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    libdrogon-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libssl-dev \
    libpq-dev \
    libsqlite3-dev \
    libmariadb-dev \
    libhiredis-dev \
    libyaml-cpp-dev \
    libbrotli-dev \
    libc-ares-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/ ./backend/

RUN cmake -S backend -B backend/build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DDLX_BUILD_TESTS=OFF \
    && cmake --build backend/build --parallel 2

ENV DLX_HOST=0.0.0.0
ENV DLX_STATIC_DIR=

EXPOSE 8080

CMD ["./backend/build/dlx_server"]
