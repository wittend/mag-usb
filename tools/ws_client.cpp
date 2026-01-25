//=======================================================================
// ws_client.cpp
//
// Simple WebSocket client for manual testing of mag-usb output.
//=======================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "websocket.h"

namespace {
constexpr uint32_t kRecvBufSize = 4096;

struct ClientHandler;
using Client = websocket::WSClient<ClientHandler, char, false, kRecvBufSize>;

struct ClientHandler {
    bool running = true;

    void onWSClose(Client &conn, uint16_t status_code, const char *reason) {
        (void)conn;
        fprintf(stderr, "WebSocket closed: %u %s\n", status_code, reason ? reason : "");
        running = false;
    }

    void onWSMsg(Client &conn, uint8_t opcode, const uint8_t *payload, uint32_t pl_len) {
        (void)conn;
        (void)opcode;
        fwrite(payload, 1, pl_len, stdout);
        if (pl_len == 0 || payload[pl_len - 1] != '\n') {
            fputc('\n', stdout);
        }
        fflush(stdout);
    }
};

} // namespace

int main(int argc, char **argv) {
    const char *server_ip = "127.0.0.1";
    int port = 8765;
    const char *path = "/";

    if (argc > 1) {
        server_ip = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    if (argc > 3) {
        path = argv[3];
    }
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %d\n", port);
        return 1;
    }

    Client client;
    ClientHandler handler;

    if (!client.wsConnect(1000, server_ip, static_cast<uint16_t>(port), path, server_ip, nullptr, nullptr, nullptr,
                          nullptr, 0, nullptr, 0)) {
        fprintf(stderr, "WebSocket connect failed: %s\n", client.getLastError());
        return 1;
    }

    while (handler.running && client.isConnected()) {
        client.poll(&handler);
        usleep(10000);
    }

    return 0;
}
