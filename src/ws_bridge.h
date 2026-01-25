//=======================================================================
// ws_bridge.h
//
// C-facing wrapper for the MengRao websocket server.
// Calls are expected from a single thread (poll + broadcast).
//=======================================================================
#ifndef WS_BRIDGE_H
#define WS_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ws_server_init(const char *bind_addr, uint16_t port);
void ws_server_shutdown(void);
void ws_server_poll(void);
void ws_server_broadcast(const char *payload, size_t payload_len);
const char *ws_server_last_error(void);
int ws_server_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // WS_BRIDGE_H
