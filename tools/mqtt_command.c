#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt_client.h"

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    const char *host = "test.mosquitto.org";
    int port = 1883;
    const char *topic = "mag-usb/data/command";
    const char *payload = "get_config";

    if (argc > 1) host = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) topic = argv[3];
    if (argc > 4) payload = argv[4];

    mqtt_client *client = mqtt_client_new();

    printf("Connecting to %s:%d...\n", host, port);
    if (mqtt_client_connect(client, host, port, 0) != 0) {
        fprintf(stderr, "Failed to connect\n");
        return 1;
    }

    if (mqtt_client_authenticate(client, NULL, NULL, "mqtt-commander") != 0) {
        fprintf(stderr, "Failed to authenticate\n");
        return 1;
    }

    printf("Publishing '%s' to %s...\n", payload, topic);
    if (mqtt_client_publish(client, topic, payload, strlen(payload)) != 0) {
        fprintf(stderr, "Failed to publish\n");
        return 1;
    }

    printf("Done.\n");
    mqtt_client_free(client);
    return 0;
}
