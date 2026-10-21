#include "mqtt_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

/* Upper bound on any single blocking send or receive.  A broker that stops
 * responding mid-packet fails the call after this instead of hanging it. */
#define MQTT_IO_TIMEOUT_SEC 5

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

static void mqtt_close_transport(mqtt_client *client);

mqtt_client* mqtt_client_new(void) {
    mqtt_client *client = (mqtt_client*)calloc(1, sizeof(mqtt_client));
    client->fd = -1;
    return client;
}

void mqtt_client_free(mqtt_client* client) {
    if (!client) return;
    mqtt_client_disconnect(client);
    mqtt_close_transport(client); // Covers a connect that failed partway
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

/* Non-blocking read of one byte.  Returns 1 if a byte was read, 0 if none is
 * waiting, and -1 if the connection is closed or failed. */
static int probe_byte(mqtt_client *client, uint8_t *byte) {
    if (client->use_tls && client->ssl) {
        int n = SSL_read(client->ssl, byte, 1);
        if (n > 0) return 1;
        int err = SSL_get_error(client->ssl, n);
        return (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) ? 0 : -1;
    }
    ssize_t n = recv(client->fd, byte, 1, 0);
    if (n > 0) return 1;
    if (n == 0) return -1;
    return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -1;
}

/* Tear down the socket and TLS state.  Safe in any state, including after a
 * failed connect, so nothing is left open. */
static void mqtt_close_transport(mqtt_client *client) {
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
    struct addrinfo hints, *res, *ai;
    struct timeval timeout = { .tv_sec = MQTT_IO_TIMEOUT_SEC, .tv_usec = 0 };
    char port_str[8];

    // A write to a connection the broker has closed raises SIGPIPE, whose
    // default action ends the process.  Ignore it so the write fails with an
    // error the caller can handle.  This is process-wide, which is fine for mag-usb.
    signal(SIGPIPE, SIG_IGN);

    // Release anything left over from an earlier connection or failed attempt
    mqtt_close_transport(client);
    free(client->host);
    client->host = strdup(host);
    client->port = port;
    client->use_tls = use_tls;
    if (!client->host) return -1;

    // Accept an IPv6 literal in brackets, e.g. "[::1]"; getaddrinfo() wants it bare
    size_t hlen = strlen(client->host);
    if (hlen > 2 && client->host[0] == '[' && client->host[hlen - 1] == ']') {
        memmove(client->host, client->host + 1, hlen - 2);
        client->host[hlen - 2] = '\0';
    }
    const char *name = client->host;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    snprintf(port_str, sizeof(port_str), "%d", port);
    if (getaddrinfo(name, port_str, &hints, &res) != 0) return -1;

    // Try each address the name resolves to until one accepts the connection
    for (ai = res; ai; ai = ai->ai_next) {
        client->fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (client->fd < 0) continue;

        setsockopt(client->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(client->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if (connect(client->fd, ai->ai_addr, ai->ai_addrlen) == 0) break;

        close(client->fd);
        client->fd = -1;
    }
    freeaddrinfo(res);
    if (client->fd < 0) return -1;

    if (use_tls) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        client->ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!client->ssl_ctx) goto fail;

        /* Verify the broker.  Without SSL_VERIFY_PEER OpenSSL accepts any
         * certificate, so use_tls gives confidentiality but no
         * authentication - anything answering on the broker's address can
         * terminate the session, read telemetry and issue commands on the
         * <topic>/command subscription. */
        SSL_CTX_set_verify(client->ssl_ctx, SSL_VERIFY_PEER, NULL);
        if (client->ca_file) {
            if (SSL_CTX_load_verify_locations(client->ssl_ctx, client->ca_file, NULL) != 1) {
                fprintf(stderr, "MQTT TLS: cannot load CA file %s\n", client->ca_file);
                goto fail;
            }
        } else {
            if (SSL_CTX_set_default_verify_paths(client->ssl_ctx) != 1) {
                fprintf(stderr, "MQTT TLS: cannot load default trust store\n");
                goto fail;
            }
        }

        client->ssl = SSL_new(client->ssl_ctx);
        if (!client->ssl) goto fail;

        /* Pin the expected identity: IP literals (IPv4 or IPv6) check the IP
         * SAN, names get SNI + hostname verification. */
        struct in_addr ipv4;
        struct in6_addr ipv6;
        if (inet_pton(AF_INET, name, &ipv4) == 1 || inet_pton(AF_INET6, name, &ipv6) == 1) {
            X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(client->ssl), name);
        } else {
            SSL_set_tlsext_host_name(client->ssl, name);
            SSL_set1_host(client->ssl, name);
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
            goto fail;
        }
        if (SSL_get_verify_result(client->ssl) != X509_V_OK) {
            fprintf(stderr, "MQTT TLS: certificate verification failed: %s\n",
                    X509_verify_cert_error_string(SSL_get_verify_result(client->ssl)));
            goto fail;
        }
    }

    client->connected = 1;
    return 0;

fail:
    mqtt_close_transport(client);
    return -1;
}

static int string_fits(const char *str) {
    return strlen(str) <= UINT16_MAX;
}

static size_t string_size(const char *str) {
    return 2 + strlen(str);
}

static void write_string(uint8_t **buf, const char *str) {
    uint16_t len = strlen(str);
    (*buf)[0] = (len >> 8) & 0xFF;
    (*buf)[1] = len & 0xFF;
    *buf += 2;
    memcpy(*buf, str, len);
    *buf += len;
}

/* Write an MQTT variable byte integer (remaining length) to ptr.  Returns the
 * number of bytes written, at most 4. */
static size_t encode_remaining_length(uint8_t *ptr, size_t len) {
    size_t n = 0;
    do {
        uint8_t byte = len % 128;
        len /= 128;
        if (len > 0) byte |= 128;
        ptr[n++] = byte;
    } while (len > 0);
    return n;
}

/* Allocate a control packet with its fixed header written and room for a body
 * of body_len bytes.  The caller fills *body, sends pkt_len bytes from pkt, and
 * frees pkt.  Returns NULL on allocation failure. */
static uint8_t *packet_new(uint8_t type, size_t body_len, uint8_t **body, size_t *pkt_len) {
    uint8_t header[5];
    size_t hlen = encode_remaining_length(header, body_len);

    *pkt_len = 1 + hlen + body_len;
    uint8_t *pkt = (uint8_t*)malloc(*pkt_len);
    if (!pkt) return NULL;

    pkt[0] = type;
    memcpy(pkt + 1, header, hlen);
    *body = pkt + 1 + hlen;
    return pkt;
}

int mqtt_client_authenticate(mqtt_client* client, const char* username, const char* password, const char* client_id) {
    // Treat empty strings as NULL for authentication
    if (username && username[0] == '\0') username = NULL;
    if (password && password[0] == '\0') password = NULL;
    if (!client_id) client_id = "mag-usb-default";

    if (!string_fits(client_id) ||
        (username && !string_fits(username)) ||
        (password && !string_fits(password))) return -1;

    // Variable header: protocol name (6), level (1), flags (1), keep alive (2)
    size_t body_len = 10 + string_size(client_id);
    if (username) body_len += string_size(username);
    if (password) body_len += string_size(password);

    uint8_t *body;
    size_t pkt_len;
    uint8_t *pkt = packet_new(0x10, body_len, &body, &pkt_len); // CONNECT packet type
    if (!pkt) return -1;
    uint8_t *ptr = body;

    write_string(&ptr, "MQTT");
    *ptr++ = 0x04; // Protocol Level (MQTT 3.1.1)

    uint8_t flags = 0x02; // Clean Session
    if (username) flags |= 0x80;
    if (password) flags |= 0x40;
    *ptr++ = flags;

    *ptr++ = 0x00; // Keep Alive MSB
    *ptr++ = 0x3C; // Keep Alive LSB (60s)

    write_string(&ptr, client_id);
    if (username) write_string(&ptr, username);
    if (password) write_string(&ptr, password);

    int rc = send_all(client, pkt, pkt_len);
    free(pkt);
    if (rc < 0) return -1;

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

    ptr += encode_remaining_length(ptr, remaining_len);

    write_string(&ptr, topic);
    memcpy(ptr, payload, payload_len);
    ptr += payload_len;

    if (send_all(client, buf, ptr - buf) < 0) return -1;

    return 0;
}

int mqtt_client_subscribe(mqtt_client* client, const char* topic) {
    if (!string_fits(topic)) return -1;

    // Variable header: packet ID (2); payload: topic filter, then requested QoS (1)
    size_t body_len = 2 + string_size(topic) + 1;

    uint8_t *body;
    size_t pkt_len;
    uint8_t *pkt = packet_new(0x82, body_len, &body, &pkt_len); // SUBSCRIBE packet type
    if (!pkt) return -1;
    uint8_t *ptr = body;

    *ptr++ = 0x00; // Packet ID MSB
    *ptr++ = 0x01; // Packet ID LSB

    write_string(&ptr, topic);
    *ptr++ = 0x00; // QoS 0

    int rc = send_all(client, pkt, pkt_len);
    free(pkt);
    if (rc < 0) return -1;

    // Wait for SUBACK (not robust, but minimal).  Byte 4 is the granted QoS,
    // where 0x80 means the broker refused the subscription.
    uint8_t ack[5];
    if (recv_all(client, ack, 5) < 0) return -1;
    if (ack[0] != 0x90 || ack[4] == 0x80) return -1;

    return 0;
}

int mqtt_client_is_connected(mqtt_client* client) {
    return client && client->connected;
}

void mqtt_client_poll(mqtt_client* client) {
    if (!client || !client->connected) return;

    // Look for a packet header without blocking the caller
    int flags = fcntl(client->fd, F_GETFL, 0);
    fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);

    uint8_t header;
    int got = probe_byte(client, &header);

    // The rest of the packet is read in blocking mode.  Those reads are
    // bounded by SO_RCVTIMEO, and any failure below drops the connection
    // because the byte stream can no longer be trusted.
    fcntl(client->fd, F_SETFL, flags);

    if (got == 0) return;
    if (got < 0) {
        mqtt_close_transport(client);
        return;
    }

    // Decode remaining length (variable byte integer).
    // MQTT 3.1.1 (2.2.3) limits this field to 4 bytes; without the cap a peer
    // holding the continuation bit drives rem_len and multiplier arbitrarily.
    size_t rem_len = 0;
    size_t multiplier = 1;
    uint8_t byte;
    int vbi_bytes = 0;
    do {
        if (recv_all(client, &byte, 1) != 0 || ++vbi_bytes > 4) {
            mqtt_close_transport(client);
            return;
        }
        rem_len += (byte & 127) * multiplier;
        multiplier *= 128;
    } while ((byte & 128) != 0);

    // Read the whole body even for packet types we do not handle, so the
    // stream stays aligned for the next packet.
    uint8_t *body = (uint8_t*)malloc(rem_len > 0 ? rem_len : 1);
    if (!body) {
        mqtt_close_transport(client);
        return;
    }
    if (recv_all(client, body, rem_len) != 0) {
        free(body);
        mqtt_close_transport(client);
        return;
    }

    // A PUBLISH body starts with the 2-byte topic length; anything shorter is
    // malformed and is ignored.  PINGRESP and other packets are discarded.
    if ((header & 0xF0) == 0x30 && rem_len >= 2) {
        uint16_t topic_len = (body[0] << 8) | body[1];
        char topic[256];
        if (topic_len < 255 && (size_t)topic_len <= rem_len - 2) {
            memcpy(topic, body + 2, topic_len);
            topic[topic_len] = '\0';
            const char *payload = (const char*)(body + 2 + topic_len);
            size_t payload_len = rem_len - 2 - topic_len;
            if (client->callback) {
                client->callback(client->user_data, topic, payload, payload_len);
            }
        }
    }

    free(body);
}

void mqtt_client_disconnect(mqtt_client* client) {
    if (!client || !client->connected) return;

    uint8_t disconnect[] = {0xE0, 0x00};
    send_all(client, disconnect, 2);

    mqtt_close_transport(client);
}
