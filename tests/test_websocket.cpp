//=======================================================================
// test_websocket.cpp
//
// Basic WebSocket server/client loopback test.
//=======================================================================
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "websocket.h"
#include "ws_bridge.h"

namespace {
constexpr uint32_t kRecvBufSize = 4096;

struct ClientHandler;
using Client = websocket::WSClient<ClientHandler, char, false, kRecvBufSize>;

struct ClientHandler {
    bool got_msg = false;
    std::string last_msg;

    void onWSClose(Client &conn, uint16_t status_code, const char *reason) {
        (void)conn;
        fprintf(stderr, "WebSocket closed: %u %s\n", status_code, reason ? reason : "");
    }

    void onWSMsg(Client &conn, uint8_t opcode, const uint8_t *payload, uint32_t pl_len) {
        (void)conn;
        (void)opcode;
        last_msg.assign(reinterpret_cast<const char *>(payload), pl_len);
        got_msg = true;
    }
};

int poll_loop(Client &client, ClientHandler &handler, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        ws_server_poll();
        client.poll(&handler);
        if (handler.got_msg) {
            return 1;
        }
        usleep(10000);
    }
    return 0;
}

} // namespace

int main(void) {
    const char *bind_addr = "127.0.0.1";
    int port = 9002;
    bool started = false;

    for (int p = 9002; p < 9010; ++p) {
        if (ws_server_init(bind_addr, static_cast<uint16_t>(p))) {
            port = p;
            started = true;
            break;
        }
    }
    if (!started) {
        fprintf(stderr, "WebSocket server init failed: %s\n", ws_server_last_error());
        return 1;
    }

    Client client;
    ClientHandler handler;
    if (!client.wsConnect(1000, bind_addr, static_cast<uint16_t>(port), "/", bind_addr, nullptr, nullptr, nullptr,
                          nullptr, 0, nullptr, 0)) {
        fprintf(stderr, "WebSocket connect failed: %s\n", client.getLastError());
        ws_server_shutdown();
        return 1;
    }

    poll_loop(client, handler, 50);

    const char *msg = "{ \"test\": true }";
    ws_server_broadcast(msg, strlen(msg));

    if (!poll_loop(client, handler, 200)) {
        fprintf(stderr, "Timed out waiting for WebSocket message.\n");
        ws_server_shutdown();
        return 1;
    }
    if (handler.last_msg != msg) {
        fprintf(stderr, "Unexpected payload: '%s'\n", handler.last_msg.c_str());
        ws_server_shutdown();
        return 1;
    }

    ws_server_shutdown();
    return 0;
}
