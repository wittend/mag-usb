// Drives mag-usb's MQTT client the way mag-usb uses it: TLS with a CA file,
// username and password, subscribe to <topic>/command, publish samples.
// Used by the local broker tests in tests/mqtt/.
//
// usage: mqtt_driver <host> <port> <ca_file|-> <user> <pass> <client_id> <topic> <publish_count>
//   "-" as ca_file means use the system trust store.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mqtt_client.h"

static int commands_seen;

static void on_msg(void *user_data, const char *topic, const char *payload, size_t len) {
    (void)user_data;
    printf("RECV [%s] %.*s\n", topic, (int)len, payload);
    if (strstr(topic, "/command")) commands_seen++;
}

int main(int argc, char **argv) {
    if (argc != 9) {
        fprintf(stderr, "usage: %s <host> <port> <ca_file|-> <user> <pass> <client_id> <topic> <publish_count>\n", argv[0]);
        return 2;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    const char *ca = strcmp(argv[3], "-") ? argv[3] : NULL;
    const char *user = argv[4];
    const char *pass = argv[5];
    const char *client_id = argv[6];
    const char *topic = argv[7];
    int count = atoi(argv[8]);

    char cmd_topic[256];
    snprintf(cmd_topic, sizeof(cmd_topic), "%s/command", topic);

    mqtt_client *m = mqtt_client_new();
    mqtt_client_set_callback(m, on_msg, NULL);
    mqtt_client_set_ca_file(m, ca);

    if (mqtt_client_connect(m, host, port, 1) != 0) {
        printf("CONNECT failed\n");
        mqtt_client_free(m);
        return 1;
    }
    printf("connected (TLS)\n");

    if (mqtt_client_authenticate(m, user, pass, client_id) != 0) {
        printf("AUTH failed\n");
        mqtt_client_free(m);
        return 1;
    }
    printf("authenticated as %s\n", user);
    printf("subscribe %s -> %d\n", cmd_topic, mqtt_client_subscribe(m, cmd_topic));

    for (int i = 0; i < count; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "{\"ts\":\"2026-10-21T12:00:%02d\",\"rt\":21.5,\"x\":1.0,\"y\":2.0,\"z\":3.0}", i);
        printf("publish %d -> %d\n", i, mqtt_client_publish(m, topic, buf, strlen(buf)));
        for (int p = 0; p < 5; p++) {
            mqtt_client_poll(m);
            usleep(100000);
        }
    }

    // Keep polling for a while so any command sent meanwhile is delivered.
    for (int p = 0; p < 20 && mqtt_client_is_connected(m); p++) {
        mqtt_client_poll(m);
        usleep(100000);
    }
    printf("still connected: %d, commands seen: %d\n", mqtt_client_is_connected(m), commands_seen);
    mqtt_client_free(m);
    return 0;
}
