#!/usr/bin/env bash
# Creates a local MQTT test broker for mag-usb: a test CA and server
# certificate, the broker config, dynsec accounts and roles, and random
# passwords.  Everything goes under run/, which is gitignored.  The system
# Mosquitto is not touched.
#
#   tests/mqtt/setup_broker.sh          first-time setup
#   tests/mqtt/setup_broker.sh --reset  stop the test broker and start over
set -euo pipefail

. "$(dirname "$0")/env.sh"

if [ "${1:-}" = "--reset" ]; then
    "$MQTT_TEST_DIR/broker.sh" stop || true
    rm -rf "$MQTT_RUN_DIR"
elif [ "${1:-}" != "" ]; then
    echo "usage: $0 [--reset]" >&2
    exit 2
fi

for cmd in mosquitto mosquitto_ctrl openssl; do
    command -v "$cmd" >/dev/null || { echo "missing $cmd (install mosquitto, mosquitto-clients, openssl)" >&2; exit 1; }
done
[ -n "$MQTT_DYNSEC_PLUGIN" ] && [ -e "$MQTT_DYNSEC_PLUGIN" ] || {
    echo "dynsec plugin not found; set MQTT_DYNSEC_PLUGIN to mosquitto_dynamic_security.so" >&2; exit 1; }
if [ -e "$MQTT_RUN_DIR" ]; then
    echo "$MQTT_RUN_DIR exists. Use --reset to start over." >&2
    exit 1
fi

umask 077
mkdir -p "$MQTT_RUN_DIR/certs"
cd "$MQTT_RUN_DIR/certs"

# Test CA and a server certificate for localhost and 127.0.0.1 / ::1.
openssl req -x509 -newkey rsa:2048 -nodes -keyout ca.key -out ca.crt -days 365 \
    -subj "/CN=mag-usb local test CA" 2>/dev/null
openssl req -newkey rsa:2048 -nodes -keyout server.key -out server.csr -subj "/CN=localhost" 2>/dev/null
printf 'subjectAltName=DNS:localhost,IP:127.0.0.1,IP:::1\nbasicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\n' > server.ext
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365 -extfile server.ext 2>/dev/null
rm -f server.csr server.ext ca.srl
cd "$MQTT_TEST_DIR"

# Random passwords for each account.  Not printed; read them from passwords.env.
{
    for v in SVC_PW STATION_PW CMD_PW VIEW_PW PROBE_PW; do
        echo "$v=$(openssl rand -hex 16)"
    done
} > "$MQTT_RUN_DIR/passwords.env"
chmod 600 "$MQTT_RUN_DIR/passwords.env"
. "$MQTT_RUN_DIR/passwords.env"

sed -e "s#@RUN_DIR@#$MQTT_RUN_DIR#g" \
    -e "s#@DYNSEC_PLUGIN@#$MQTT_DYNSEC_PLUGIN#g" \
    -e "s#@PORT_PLAIN@#$MQTT_PORT_PLAIN#g" \
    -e "s#@PORT_TLS@#$MQTT_PORT_TLS#g" \
    -e "s#@PORT_WS@#$MQTT_PORT_WS#g" \
    "$MQTT_TEST_DIR/mosquitto.conf.in" > "$MQTT_RUN_DIR/mosquitto.conf"

# The dynsec file must exist before the broker starts.
mosquitto_ctrl dynsec init "$MQTT_RUN_DIR/dynamic-security.json" portal-svc "$SVC_PW" >/dev/null 2>&1
chmod 600 "$MQTT_RUN_DIR/dynamic-security.json"

"$MQTT_TEST_DIR/broker.sh" start

# mosquitto_ctrl exits 0 even when the broker refuses a command, so check the output.
ctrl() {
    local out
    out="$(mosquitto_ctrl -h 127.0.0.1 -p "$MQTT_PORT_PLAIN" -u portal-svc -P "$SVC_PW" dynsec "$@" 2>&1)" || true
    if printf '%s' "$out" | grep -qi 'error'; then
        echo "dynsec $* failed: $out" >&2
        exit 1
    fi
}

# Defaults deny everything; each permission below is granted explicitly.
ctrl setDefaultACLAccess publishClientSend deny
ctrl setDefaultACLAccess publishClientReceive deny
ctrl setDefaultACLAccess subscribe deny
ctrl setDefaultACLAccess unsubscribe deny

# Station: publishes samples and receives commands.
ctrl createRole station-test
ctrl addRoleACL station-test publishClientSend hamsci/mag/TEST/data allow
ctrl addRoleACL station-test subscribeLiteral hamsci/mag/TEST/data/command allow
ctrl addRoleACL station-test publishClientReceive hamsci/mag/TEST/data/command allow
ctrl createClient station-TEST -p "$STATION_PW"
ctrl addClientRole station-TEST station-test

# Command issuer: publishes commands only.
ctrl createRole command-issuer
ctrl addRoleACL command-issuer publishClientSend hamsci/mag/TEST/data/command allow
ctrl createClient cmd-TEST -p "$CMD_PW"
ctrl addClientRole cmd-TEST command-issuer

# Viewer: reads samples only.
ctrl createRole viewer
ctrl addRoleACL viewer subscribeLiteral hamsci/mag/TEST/data allow
ctrl addRoleACL viewer publishClientReceive hamsci/mag/TEST/data allow
ctrl createClient viewer-TEST -p "$VIEW_PW"
ctrl addClientRole viewer-TEST viewer

# Probe: can see every topic.  Used by acl_test.sh to check what others send.
ctrl createRole probe
for t in hamsci/mag/TEST/data hamsci/mag/TEST/data/command hamsci/mag/TEST/other; do
    ctrl addRoleACL probe subscribeLiteral "$t" allow
    ctrl addRoleACL probe publishClientReceive "$t" allow
done
ctrl createClient probe-TEST -p "$PROBE_PW"
ctrl addClientRole probe-TEST probe

echo
echo "Test broker is set up and running."
echo "  plain:     127.0.0.1:$MQTT_PORT_PLAIN"
echo "  TLS:       127.0.0.1:$MQTT_PORT_TLS  (CA: $MQTT_RUN_DIR/certs/ca.crt)"
echo "  WebSocket: 127.0.0.1:$MQTT_PORT_WS"
echo "Next: tests/mqtt/acl_test.sh"
