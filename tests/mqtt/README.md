# Local MQTT test broker

These scripts start a Mosquitto broker on this machine for testing mag-usb's
MQTT client against a real broker: TLS with a CA file, username/password
accounts, and per-role topic rules.  It's the same access model as the
station setup described in [Configuration](../../docs/Configuration.md).

The broker uses its own ports (18830, 18834, 19001), so it doesn't clash with
a system Mosquitto on 1883.  Everything it creates goes under `tests/mqtt/run/`,
which is gitignored and holds the passwords, the test CA and the dynsec file.
Don't use it for anything real.

## Requirements

Debian/Ubuntu: `sudo apt install mosquitto mosquitto-clients openssl`.
The dynsec plugin (`mosquitto_dynamic_security.so`) comes with `mosquitto`.
The scripts find it under `/usr/lib` or `/usr/local/lib`, or set
`MQTT_DYNSEC_PLUGIN` to its path.

## Setup

```sh
tests/mqtt/setup_broker.sh          # first time: certs, config, accounts, roles; starts the broker
tests/mqtt/broker.sh status         # running or not
tests/mqtt/broker.sh stop | start
tests/mqtt/setup_broker.sh --reset # throw everything away and start over
```

Setup creates these accounts, all with random passwords in
`tests/mqtt/run/passwords.env`:

| Account | Role | Can do |
|---|---|---|
| `station-TEST` | `station-test` | publish `hamsci/mag/TEST/data`; receive `hamsci/mag/TEST/data/command` |
| `cmd-TEST` | `command-issuer` | publish `hamsci/mag/TEST/data/command` |
| `viewer-TEST` | `viewer` | read `hamsci/mag/TEST/data` |
| `probe-TEST` | `probe` | see every topic (used by the ACL tests) |

The default ACL denies everything not granted by a role, and anonymous
connections are refused.

## Checking the access rules

```sh
tests/mqtt/acl_test.sh
```

Each check sends one message and reports whether a subscriber received it.
It exits non-zero if any check fails.

## Driving mag-usb's MQTT client

`mqtt_driver.c` uses the same client code as mag-usb.  Build it from the
repository root:

```sh
gcc -std=gnu11 -Wall -Wextra -D_GNU_SOURCE -Isrc \
    tests/mqtt/mqtt_driver.c src/mqtt_client.c -lssl -lcrypto \
    -o tests/mqtt/run/mqtt_driver
. tests/mqtt/run/passwords.env
tests/mqtt/run/mqtt_driver localhost 18834 tests/mqtt/run/certs/ca.crt \
    station-TEST "$STATION_PW" mag-usb-TEST hamsci/mag/TEST/data 5
```

The arguments are host, port, CA file (or `-` for the system trust store),
username, password, client id, topic, and the number of samples to publish.
The driver subscribes to `<topic>/command`, so you can send a command from
another terminal:

```sh
. tests/mqtt/run/passwords.env
mosquitto_pub -h 127.0.0.1 -p 18830 -u cmd-TEST -P "$CMD_PW" \
    -t hamsci/mag/TEST/data/command -m get_config
```

Negative cases to try by hand:
- Leave out the CA file (`-`): the connection fails certificate verification.
- Use a wrong password: the driver prints `AUTH failed`.
- Stop the broker mid-run (`broker.sh stop`): later publishes return -1, and
  the driver reports it is no longer connected.

## What this doesn't cover

- The `mag-usb` binary itself, which needs the I2C hardware.  The driver uses
  the same client code.
- The dashboard over WebSocket (port 19001).  The listener is up, but nothing
  here connects to it.
- Reconnecting after a drop.  mag-usb doesn't reconnect yet.
