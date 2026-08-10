#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mqtt_client mqtt_client;

typedef void (*mqtt_msg_callback)(void *user_data, const char *topic, const char *payload, size_t payload_len);

mqtt_client* mqtt_client_new(void);
void mqtt_client_free(mqtt_client* client);

void mqtt_client_set_callback(mqtt_client* client, mqtt_msg_callback cb, void *user_data);

/* Optional: CA bundle for TLS peer verification.  When unset, the
 * system default trust store is used.  Must be called before
 * mqtt_client_connect().  Returns 0 on success. */
int mqtt_client_set_ca_file(mqtt_client* client, const char* ca_file);

int mqtt_client_connect(mqtt_client* client, 
                        const char* host, 
                        int port, 
                        int use_tls);

int mqtt_client_authenticate(mqtt_client* client,
                             const char* username,
                             const char* password,
                             const char* client_id);

int mqtt_client_publish(mqtt_client* client,
                        const char* topic,
                        const char* payload,
                        size_t payload_len);

int mqtt_client_subscribe(mqtt_client* client,
                          const char* topic);

int mqtt_client_is_connected(mqtt_client* client);
void mqtt_client_poll(mqtt_client* client);
void mqtt_client_disconnect(mqtt_client* client);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_H
