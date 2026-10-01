#include "mqtt_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

struct mqtt_client {
    int fd;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    int use_tls;
    int connected;
    char *ca_file;
    char *host;
    int port;
    mqtt_msg_callback callback;
    void *user_data;
};

mqtt_client* mqtt_client_new(void) {
    mqtt_client *client = (mqtt_client*)calloc(1, sizeof(mqtt_client));
    client->fd = -1;
    return client;
}

void mqtt_client_free(mqtt_client* client) {
    if (!client) return;
    mqtt_client_disconnect(client);
    if (client->host) free(client->host);
    if (client->ca_file) free(client->ca_file);
    free(client);
}

static int send_all(mqtt_client *client, const void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        int n;
        if (client->use_tls && client->ssl) {
            n = SSL_write(client->ssl, (const char*)buf + total, len - total);
        } else {
            n = send(client->fd, (const char*)buf + total, len - total, 0);
        }
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

static int recv_all(mqtt_client *client, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        int n;
        if (client->use_tls && client->ssl) {
            n = SSL_read(client->ssl, (char*)buf + total, len - total);
        } else {
            n = recv(client->fd, (char*)buf + total, len - total, 0);
        }
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

int mqtt_client_set_ca_file(mqtt_client* client, const char* ca_file) {
    if (!client) return -1;
    if (client->ca_file) { free(client->ca_file); client->ca_file = NULL; }
    if (ca_file && ca_file[0] != '\0') {
        client->ca_file = strdup(ca_file);
        if (!client->ca_file) return -1;
    }
    return 0;
}

void mqtt_client_set_callback(mqtt_client* client, mqtt_msg_callback cb, void *user_data) {
    if (!client) return;
    client->callback = cb;
    client->user_data = user_data;
}

int mqtt_client_connect(mqtt_client* client, const char* host, int port, int use_tls) {
    struct hostent *server;
    struct sockaddr_in serv_addr;

    client->host = strdup(host);
    client->port = port;
    client->use_tls = use_tls;

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->fd < 0) return -1;

    server = gethostbyname(host);
    if (server == NULL) return -1;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(client->fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(client->fd);
        client->fd = -1;
        return -1;
    }

    if (use_tls) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        client->ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!client->ssl_ctx) return -1;

        /* Verify the broker.  Without SSL_VERIFY_PEER OpenSSL accepts any
         * certificate, so use_tls gives confidentiality but no
         * authentication - anything answering on the broker's address can
         * terminate the session, read telemetry and issue commands on the
         * <topic>/command subscription. */
        SSL_CTX_set_verify(client->ssl_ctx, SSL_VERIFY_PEER, NULL);
        if (client->ca_file) {
            if (SSL_CTX_load_verify_locations(client->ssl_ctx, client->ca_file, NULL) != 1) {
                fprintf(stderr, "MQTT TLS: cannot load CA file %s\n", client->ca_file);
                return -1;
            }
        } else {
            if (SSL_CTX_set_default_verify_paths(client->ssl_ctx) != 1) {
                fprintf(stderr, "MQTT TLS: cannot load default trust store\n");
                return -1;
            }
        }

        client->ssl = SSL_new(client->ssl_ctx);
        if (!client->ssl) return -1;

        /* Pin the expected identity: IP literals check the IP SAN,
         * names get SNI + hostname verification. */
        struct in_addr ipv4;
        if (inet_pton(AF_INET, host, &ipv4) == 1) {
            X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(client->ssl), host);
        } else {
            SSL_set_tlsext_host_name(client->ssl, host);
            SSL_set1_host(client->ssl, host);
        }

        SSL_set_fd(client->ssl, client->fd);
        if (SSL_connect(client->ssl) <= 0) {
            unsigned long e = ERR_get_error();
            long vr = SSL_get_verify_result(client->ssl);
            if (vr != X509_V_OK) {
                fprintf(stderr, "MQTT TLS: certificate verification failed: %s\n",
                        X509_verify_cert_error_string(vr));
            } else if (e) {
                fprintf(stderr, "MQTT TLS: handshake failed: %s\n",
                        ERR_error_string(e, NULL));
            }
            return -1;
        }
        if (SSL_get_verify_result(client->ssl) != X509_V_OK) {
            fprintf(stderr, "MQTT TLS: certificate verification failed: %s\n",
                    X509_verify_cert_error_string(SSL_get_verify_result(client->ssl)));
            return -1;
        }
    }

    client->connected = 1;
    return 0;
}

static void write_string(uint8_t **buf, const char *str) {
    uint16_t len = strlen(str);
    (*buf)[0] = (len >> 8) & 0xFF;
    (*buf)[1] = len & 0xFF;
    *buf += 2;
    memcpy(*buf, str, len);
    *buf += len;
}

int mqtt_client_authenticate(mqtt_client* client, const char* username, const char* password, const char* client_id) {
    uint8_t buf[512];
    uint8_t *ptr = buf;
    
    // Treat empty strings as NULL for authentication
    if (username && username[0] == '\0') username = NULL;
    if (password && password[0] == '\0') password = NULL;

    // MQTT CONNECT header
    *ptr++ = 0x10; // CONNECT packet type
    
    // Remaining length (placeholder)
    uint8_t *len_ptr = ptr++;
    
    // Protocol Name
    write_string(&ptr, "MQTT");
    *ptr++ = 0x04; // Protocol Level (MQTT 3.1.1)
    
    uint8_t flags = 0x02; // Clean Session
    if (username) flags |= 0x80;
    if (password) flags |= 0x40;
    *ptr++ = flags;
    
    *ptr++ = 0x00; // Keep Alive MSB
    *ptr++ = 0x3C; // Keep Alive LSB (60s)
    
    write_string(&ptr, client_id ? client_id : "mag-usb-default");
    if (username) write_string(&ptr, username);
    if (password) write_string(&ptr, password);
    
    *len_ptr = (ptr - len_ptr - 1); // Only works for lengths < 128
    
    if (send_all(client, buf, ptr - buf) < 0) return -1;
    
    // Read CONNACK
    uint8_t ack[4];
    if (recv_all(client, ack, 4) < 0) return -1;
    if (ack[0] != 0x20 || ack[3] != 0x00) return -1;
    
    return 0;
}

int mqtt_client_publish(mqtt_client* client, const char* topic, const char* payload, size_t payload_len) {
    uint8_t buf[2048];
    uint8_t *ptr = buf;
    
    *ptr++ = 0x30; // PUBLISH packet type, QoS 0
    
    size_t remaining_len = 2 + strlen(topic) + payload_len;
    if (remaining_len > 2000) return -1; // Basic safety
    
    // Remaining length encoding (MQTT variable byte integer)
    size_t x = remaining_len;
    do {
        uint8_t encoded_byte = x % 128;
        x = x / 128;
        if (x > 0) {
            encoded_byte |= 128;
        }
        *ptr++ = encoded_byte;
    } while (x > 0);
    
    write_string(&ptr, topic);
    memcpy(ptr, payload, payload_len);
    ptr += payload_len;
    
    if (send_all(client, buf, ptr - buf) < 0) return -1;
    
    return 0;
}

int mqtt_client_subscribe(mqtt_client* client, const char* topic) {
    uint8_t buf[512];
    uint8_t *ptr = buf;
    
    *ptr++ = 0x82; // SUBSCRIBE packet type
    uint8_t *len_ptr = ptr++;
    
    *ptr++ = 0x00; // Packet ID MSB
    *ptr++ = 0x01; // Packet ID LSB
    
    write_string(&ptr, topic);
    *ptr++ = 0x00; // QoS 0
    
    *len_ptr = (ptr - len_ptr - 1);
    
    if (send_all(client, buf, ptr - buf) < 0) return -1;
    
    // Wait for SUBACK (not robust, but minimal)
    uint8_t ack[5];
    if (recv_all(client, ack, 5) < 0) return -1;
    if (ack[0] != 0x90) return -1;
    
    return 0;
}

int mqtt_client_is_connected(mqtt_client* client) {
    return client && client->connected;
}

void mqtt_client_poll(mqtt_client* client) {
    if (!client || !client->connected) return;

    // Set socket to non-blocking
    int flags = fcntl(client->fd, F_GETFL, 0);
    fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);

    uint8_t header;
    int n;
    if (client->use_tls && client->ssl) {
        n = SSL_read(client->ssl, &header, 1);
    } else {
        n = recv(client->fd, &header, 1, 0);
    }

    if (n > 0) {
        // We have data! Switch back to blocking for the rest of the packet.
        fcntl(client->fd, F_SETFL, flags);

        if ((header & 0xF0) == 0x30) { // PUBLISH
            // Decode remaining length (variable byte integer).
            // MQTT 3.1.1 (2.2.3) limits this field to 4 bytes; without the
            // cap a peer holding the continuation bit drives rem_len and
            // multiplier arbitrarily.
            size_t rem_len = 0;
            size_t multiplier = 1;
            uint8_t byte;
            int vbi_bytes = 0;
            do {
                if (recv_all(client, &byte, 1) != 0) return;
                if (++vbi_bytes > 4) return; // malformed length field
                rem_len += (byte & 127) * multiplier;
                multiplier *= 128;
            } while ((byte & 128) != 0);

            // A PUBLISH body starts with the 2-byte topic length; anything
            // shorter is malformed (and rem_len - 2 below would underflow).
            if (rem_len < 2) return;

            uint8_t *msg = (uint8_t*)malloc(rem_len);
            if (!msg) return;
            if (recv_all(client, msg, rem_len) == 0) {
                uint16_t topic_len = (msg[0] << 8) | msg[1];
                char topic[256];
                if (topic_len < 255 && (size_t)topic_len <= rem_len - 2) {
                    memcpy(topic, msg + 2, topic_len);
                    topic[topic_len] = '\0';
                    const char *payload = (const char*)(msg + 2 + topic_len);
                    size_t payload_len = rem_len - 2 - topic_len;
                    if (client->callback) {
                        client->callback(client->user_data, topic, payload, payload_len);
                    }
                }
            }
            free(msg);
        } else if ((header & 0xF0) == 0xC0) { // PINGRESP
            // Ignore
        }
        
        // Restore non-blocking state if needed (but we are going to reset it anyway at the end)
        fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);
    }

    // Reset socket to blocking
    fcntl(client->fd, F_SETFL, flags);
}

void mqtt_client_disconnect(mqtt_client* client) {
    if (!client || !client->connected) return;
    
    uint8_t disconnect[] = {0xE0, 0x00};
    send_all(client, disconnect, 2);
    
    if (client->ssl) {
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
        client->ssl = NULL;
    }
    if (client->ssl_ctx) {
        SSL_CTX_free(client->ssl_ctx);
        client->ssl_ctx = NULL;
    }
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }
    client->connected = 0;
}
