#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mqtt_client.h"

void on_message(void *user_data, const char *topic, const char *payload, size_t payload_len) {
    char *buf = malloc(payload_len + 1);
    memcpy(buf, payload, payload_len);
    buf[payload_len] = '\0';
    printf("[%s] %s\n", topic, buf);
    free(buf);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    const char *host = "test.mosquitto.org";
    int port = 1883;
    const char *topic = "mag-usb/data";

    if (argc > 1) host = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) topic = argv[3];

    mqtt_client *client = mqtt_client_new();
    mqtt_client_set_callback(client, on_message, NULL);

    printf("Connecting to %s:%d...\n", host, port);
    if (mqtt_client_connect(client, host, port, 0) != 0) {
        fprintf(stderr, "Failed to connect\n");
        return 1;
    }

    if (mqtt_client_authenticate(client, NULL, NULL, "mqtt-listener") != 0) {
        fprintf(stderr, "Failed to authenticate\n");
        return 1;
    }

    printf("Subscribing to %s...\n", topic);
    mqtt_client_subscribe(client, topic);

    // Also subscribe to command topic to see requests
    char cmd_topic[256];
    snprintf(cmd_topic, sizeof(cmd_topic), "%s/command", topic);
    mqtt_client_subscribe(client, cmd_topic);

    printf("Listening... (Press Ctrl+C to stop)\n");
    while (1) {
        mqtt_client_poll(client);
        usleep(100000);
    }

    mqtt_client_free(client);
    return 0;
}
