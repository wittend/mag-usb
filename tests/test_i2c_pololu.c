#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>

#include "i2c-pololu.h"

typedef struct {
    int sock;                // mock device socket (peer of adapter->fd)
    volatile bool running;
} mock_ctx_t;

static void *mock_thread(void *arg)
{
    mock_ctx_t *ctx = (mock_ctx_t *)arg;
    uint8_t buf[512];
    while (ctx->running) {
        ssize_t n = read(ctx->sock, buf, sizeof(buf));
        if (n <= 0) {
            if (errno == EINTR) continue;
            break;
        }
        // Simple dispatcher based on first byte command
        if (buf[0] == CMD_I2C_WRITE && n >= 3) {
            // Always respond with ERROR_NONE
            uint8_t resp = ERROR_NONE;
            write(ctx->sock, &resp, 1);
        } else if (buf[0] == CMD_I2C_READ && n >= 3) {
            uint8_t size = buf[2];
            uint8_t resp[1 + 255];
            resp[0] = ERROR_NONE;
            for (int i = 0; i < size; ++i) resp[1 + i] = (uint8_t)(0xA0 + i);
            write(ctx->sock, resp, 1 + size);
        } else if (buf[0] == CMD_I2C_WRITE_AND_READ && n >= 5) {
            uint8_t size = buf[3];
            uint8_t resp[1 + 255];
            resp[0] = ERROR_NONE;
            for (int i = 0; i < size; ++i) resp[1 + i] = (uint8_t)(0xB0 + i);
            write(ctx->sock, resp, 1 + size);
        } else if (buf[0] == CMD_SET_I2C_MODE && n >= 2) {
            // No response expected by code; just continue
        } else if (buf[0] == CMD_CLEAR_BUS) {
            // No response expected by code; just continue
        } else if (buf[0] == CMD_GET_DEVICE_INFO) {
            // Send 28 bytes total (length + payload-1 already read scheme). The code first reads a length byte, then length-1.
            // Here, we need to first send length byte, then payload on the next read.
            uint8_t length = 28;
            write(ctx->sock, &length, 1);
            struct __attribute__((packed)) raw_info {
                uint8_t version;
                uint16_t vendor_id;
                uint16_t product_id;
                uint16_t firmware_version_bcd;
                char firmware_modification[8];
                char serial_number[12];
            } payload;
            payload.version = 0;
            payload.vendor_id = 0x1FFB;   // example
            payload.product_id = 0x00C0;  // example
            payload.firmware_version_bcd = 0x0102; // 1.02
            memcpy(payload.firmware_modification, "-\0\0\0\0\0\0\0", 8);
            for (int i = 0; i < 12; ++i) payload.serial_number[i] = (char)(i + 1);
            write(ctx->sock, &payload, sizeof(payload));
        } else if (buf[0] == 0x91 && n == 128 * 3) { // scan prebuilt command sequence
            // Respond with 128 bytes, each being error code per address. Mark addresses 0x10, 0x1A, 0x50 as present (ERROR_NONE), others ERROR_ADDRESS_NACK
            uint8_t responses[128];
            for (int i = 0; i < 128; ++i) responses[i] = ERROR_ADDRESS_NACK;
            responses[0x10] = ERROR_NONE;
            responses[0x1A] = ERROR_NONE;
            responses[0x50] = ERROR_NONE;
            write(ctx->sock, responses, 128);
        } else {
            // Unknown command: do nothing
        }
    }
    return NULL;
}

static void start_mock(mock_ctx_t *ctx, int sock)
{
    ctx->sock = sock;
    ctx->running = true;
    pthread_t tid;
    pthread_create(&tid, NULL, mock_thread, ctx);
    pthread_detach(tid);
}

static void stop_mock(mock_ctx_t *ctx)
{
    ctx->running = false;
    // Closing the socket will break the loop
    if (ctx->sock >= 0) close(ctx->sock);
}

static int tests_failed = 0;
#define ASSERT_TRUE(cond, msg) do { if (!(cond)) { fprintf(stderr, "ASSERT FAILED: %s\n", msg); tests_failed++; } } while(0)
#define ASSERT_EQ_INT(a,b,msg) do { if ((a)!=(b)) { fprintf(stderr, "ASSERT FAILED: %s (got %d expected %d)\n", msg, (int)(a), (int)(b)); tests_failed++; } } while(0)
#define ASSERT_MEMEQ(a,b,n,msg) do { if (memcmp((a),(b),(n))!=0) { fprintf(stderr, "ASSERT FAILED: %s (memory compare failed)\n", msg); tests_failed++; } } while(0)

static void test_init_and_connection_bits()
{
    i2c_pololu_adapter ad;
    ASSERT_EQ_INT(i2c_pololu_init(&ad), 0, "init returns 0");
    ASSERT_TRUE(!i2c_pololu_is_connected(&ad), "not connected after init");

    // Fake connect using socketpair
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    ad.fd = sv[0];
    ASSERT_TRUE(i2c_pololu_is_connected(&ad), "connected when fd >=0");
    i2c_pololu_disconnect(&ad);
    ASSERT_TRUE(!i2c_pololu_is_connected(&ad), "disconnected sets fd=-1");
    close(sv[1]);

    // Connect negative: try opening invalid path
    ASSERT_EQ_INT(i2c_pololu_connect(&ad, "/dev/does_not_exist"), -1, "connect invalid path fails");
}

static void test_error_string()
{
    ASSERT_TRUE(strstr(i2c_pololu_error_string(ERROR_NACK), "NACK") != NULL, "error string contains NACK");
    ASSERT_TRUE(strstr(i2c_pololu_error_string(9999), "Unknown") != NULL, "unknown error string");
}

static void test_write_read_sequences()
{
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    mock_ctx_t mock = {0};
    start_mock(&mock, sv[1]);

    i2c_pololu_adapter ad;
    i2c_pololu_init(&ad);
    ad.fd = sv[0];

    // write_to
    uint8_t data[3] = {0x11, 0x22, 0x33};
    int rc = i2c_pololu_write_to(&ad, 0x50, 0x0A, data, 3);
    ASSERT_EQ_INT(rc, 3, "write_to returns size");

    // read_from
    uint8_t rbuf[4] = {0};
    rc = i2c_pololu_read_from(&ad, 0x1A, 0x00, rbuf, 4);
    ASSERT_EQ_INT(rc, 4, "read_from returns size");
    uint8_t expected1[4] = {0xA0,0xA1,0xA2,0xA3};
    ASSERT_MEMEQ(rbuf, expected1, 4, "read_from data matches");

    // write_and_read_from
    uint8_t rbuf2[5] = {0};
    rc = i2c_pololu_write_and_read_from(&ad, 0x1A, 0x10, rbuf2, 5);
    ASSERT_EQ_INT(rc, 5, "write_and_read_from returns size");
    uint8_t expected2[5] = {0xB0,0xB1,0xB2,0xB3,0xB4};
    ASSERT_MEMEQ(rbuf2, expected2, 5, "write_and_read_from data matches");

    stop_mock(&mock);
    i2c_pololu_disconnect(&ad);
}

static void test_frequency_and_clear_bus()
{
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    mock_ctx_t mock = {0};
    start_mock(&mock, sv[1]);

    i2c_pololu_adapter ad; i2c_pololu_init(&ad); ad.fd = sv[0];

    ASSERT_EQ_INT(i2c_pololu_set_frequency(&ad, 50), 0, "set_frequency 50 kHz");
    ASSERT_EQ_INT(i2c_pololu_set_frequency(&ad, 100), 0, "set_frequency 100 kHz");
    ASSERT_EQ_INT(i2c_pololu_set_frequency(&ad, 400), 0, "set_frequency 400 kHz");
    ASSERT_EQ_INT(i2c_pololu_set_frequency(&ad, 1000), 0, "set_frequency 1000 kHz");

    ASSERT_EQ_INT(i2c_pololu_clear_bus(&ad), 0, "clear_bus");

    stop_mock(&mock);
    i2c_pololu_disconnect(&ad);
}

static void test_device_info_and_scan()
{
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    mock_ctx_t mock = {0};
    start_mock(&mock, sv[1]);

    i2c_pololu_adapter ad; i2c_pololu_init(&ad); ad.fd = sv[0];

    i2c_pololu_device_info info;
    int rc = i2c_pololu_get_device_info(&ad, &info);
    ASSERT_EQ_INT(rc, 0, "get_device_info returns 0");
    ASSERT_EQ_INT(info.vendor_id, 0x1FFB, "vendor id parsed");
    ASSERT_EQ_INT(info.product_id, 0x00C0, "product id parsed");
    ASSERT_TRUE(strcmp(info.firmware_version, "1.02") == 0, "firmware version parsed");
    ASSERT_TRUE(strlen(info.serial_number) > 0, "serial number formatted");

    uint8_t found[4] = {0};
    rc = i2c_pololu_scan(&ad, found, 4);
    ASSERT_EQ_INT(rc, 3, "scan found 3 devices");
    ASSERT_EQ_INT(found[0], 0x10, "scan first addr");
    ASSERT_EQ_INT(found[1], 0x1A, "scan second addr");
    ASSERT_EQ_INT(found[2], 0x50, "scan third addr");

    stop_mock(&mock);
    i2c_pololu_disconnect(&ad);
}

int main(void)
{
    test_init_and_connection_bits();
    test_error_string();
    test_write_read_sequences();
    test_frequency_and_clear_bus();
    test_device_info_and_scan();

    if (tests_failed) {
        fprintf(stderr, "\nTESTS FAILED: %d\n", tests_failed);
        return 1;
    }
    printf("All tests passed.\n");
    return 0;
}
