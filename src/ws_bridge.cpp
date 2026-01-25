//=======================================================================
// ws_bridge.cpp
//
// C-facing wrapper for the MengRao websocket server.
//=======================================================================
#include "ws_bridge.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "websocket.h"

namespace {
constexpr uint32_t kRecvBufSize = 4096;
constexpr uint32_t kMaxConns = 16;

struct ConnState {
    bool connected = false;
};

struct Handler;
using Connection = websocket::WSConnection<Handler, ConnState, false, kRecvBufSize, false>;
using Server = websocket::WSServer<Handler, ConnState, false, kRecvBufSize, kMaxConns>;

struct Handler {
    std::vector<Connection*> *connections = nullptr;

    bool onWSConnect(Connection &conn, const char *request_uri, const char *host, const char *origin,
                     const char *protocol, const char *extensions, char *resp_protocol,
                     uint32_t resp_protocol_size, char *resp_extensions, uint32_t resp_extensions_size) {
        (void)request_uri;
        (void)host;
        (void)origin;
        (void)protocol;
        (void)extensions;
        (void)resp_protocol;
        (void)resp_protocol_size;
        (void)resp_extensions;
        (void)resp_extensions_size;
        if (connections) {
            connections->push_back(&conn);
        }
        return true;
    }

    void onWSClose(Connection &conn, uint16_t status_code, const char *reason) {
        (void)status_code;
        (void)reason;
        if (!connections) {
            return;
        }
        auto it = std::find(connections->begin(), connections->end(), &conn);
        if (it != connections->end()) {
            connections->erase(it);
        }
    }

    void onWSMsg(Connection &conn, uint8_t opcode, const uint8_t *payload, uint32_t pl_len) {
        (void)conn;
        (void)opcode;
        (void)payload;
        (void)pl_len;
    }

    void onWSSegment(Connection &conn, uint8_t opcode, const uint8_t *payload, uint32_t pl_len,
                     uint32_t pl_start_idx, bool fin) {
        (void)conn;
        (void)opcode;
        (void)payload;
        (void)pl_len;
        (void)pl_start_idx;
        (void)fin;
    }
};

Server g_server;
Handler g_handler;
std::vector<Connection*> g_connections;
std::string g_last_error;
bool g_running = false;

} // namespace

int ws_server_init(const char *bind_addr, uint16_t port) {
    g_connections.clear();
    g_handler.connections = &g_connections;
    if (bind_addr == nullptr || bind_addr[0] == '\0') {
        bind_addr = "0.0.0.0";
    }
    if (!g_server.init(bind_addr, port)) {
        g_last_error = g_server.getLastError();
        g_running = false;
        return 0;
    }
    g_running = true;
    return 1;
}

void ws_server_shutdown(void) {
    if (!g_running) {
        return;
    }
    for (Connection *conn : g_connections) {
        if (conn && conn->isConnected()) {
            conn->close(1000, "shutdown");
        }
    }
    g_connections.clear();
    g_running = false;
}

void ws_server_poll(void) {
    if (!g_running) {
        return;
    }
    g_server.poll(&g_handler);
}

void ws_server_broadcast(const char *payload, size_t payload_len) {
    if (!g_running || payload == nullptr) {
        return;
    }
    if (payload_len == 0) {
        payload_len = strlen(payload);
    }
    for (auto it = g_connections.begin(); it != g_connections.end();) {
        Connection *conn = *it;
        if (!conn || !conn->isConnected()) {
            it = g_connections.erase(it);
            continue;
        }
        conn->send(websocket::OPCODE_TEXT, reinterpret_cast<const uint8_t *>(payload),
                   static_cast<uint32_t>(payload_len));
        ++it;
    }
}

const char *ws_server_last_error(void) {
    return g_last_error.c_str();
}

int ws_server_is_running(void) {
    return g_running ? 1 : 0;
}
