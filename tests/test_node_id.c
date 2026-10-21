// Tests for node_id_is_valid(): the rules for node_id in config.toml and -N.
#include <stdio.h>
#include <string.h>
#include "node_id.h"

static int failures = 0;

static void expect(int want, const char *s) {
    int got = node_id_is_valid(s);
    if (got != want) {
        printf("FAIL: node_id_is_valid(\"%s\") = %d, want %d\n", s ? s : "(null)", got, want);
        failures++;
    }
}

int main(void) {
    char longest[NODE_ID_MAX_LEN + 1];
    char too_long[NODE_ID_MAX_LEN + 2];
    memset(longest, 'A', NODE_ID_MAX_LEN);
    longest[NODE_ID_MAX_LEN] = '\0';
    memset(too_long, 'A', NODE_ID_MAX_LEN + 1);
    too_long[NODE_ID_MAX_LEN + 1] = '\0';

    expect(1, "KD0EAG");
    expect(1, "station-01_a@home.net");
    expect(1, "x");
    expect(1, longest);
    expect(0, too_long);
    expect(0, "");
    expect(0, NULL);
    expect(0, "has space");
    expect(0, "tab\there");
    expect(0, "quote\"here");
    expect(0, "back\\slash");
    expect(0, "caf\xC3\xA9");  // non-ASCII bytes

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("node_id tests passed\n");
    return 0;
}
