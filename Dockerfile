# Build Stage
FROM ubuntu:noble AS builder

## Install build tools
RUN apt-get update -q && \
    apt-get install -y build-essential cmake && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /mag-usb
COPY . .

## Build mag-usb executable
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_WEBSOCKET=ON && \
    cmake --build build --target mag-usb

# Deploy Stage
FROM ubuntu:noble

## Port for the physical device
ENV HW_PORT=1234

## Install nano (to edit config.toml) and socat (virtual TTY)
RUN apt-get update -q && \
    apt-get install -y nano socat && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

## Add directory for storing .log files
RUN mkdir logs

## Copy mag-usb binary and config.toml from build stage (reduce container size)
COPY --from=builder /mag-usb/build/mag-usb /app/mag-usb
COPY --from=builder /mag-usb/src/config.toml /etc/mag-usb/config.toml

## Copy entrypoint script
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENV TERM=xterm-256color

CMD ["/entrypoint.sh"]
