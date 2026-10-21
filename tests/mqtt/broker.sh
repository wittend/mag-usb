#!/usr/bin/env bash
# Starts or stops the local test broker created by setup_broker.sh.
#   tests/mqtt/broker.sh start|stop|status
set -euo pipefail

. "$(dirname "$0")/env.sh"

PIDFILE="$MQTT_RUN_DIR/broker.pid"

running_pid() {
    [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null && cat "$PIDFILE"
}

case "${1:-}" in
start)
    if pid="$(running_pid)" && [ -n "$pid" ]; then
        echo "already running (pid $pid)"
        exit 0
    fi
    [ -f "$MQTT_RUN_DIR/mosquitto.conf" ] || { echo "run setup_broker.sh first" >&2; exit 1; }
    if ss -ltn | grep -q ":$MQTT_PORT_PLAIN "; then
        echo "port $MQTT_PORT_PLAIN is already in use by something else" >&2
        exit 1
    fi
    mosquitto -c "$MQTT_RUN_DIR/mosquitto.conf" > "$MQTT_RUN_DIR/broker.log" 2>&1 &
    echo $! > "$PIDFILE"
    sleep 1
    if ! running_pid >/dev/null; then
        echo "broker failed to start; see $MQTT_RUN_DIR/broker.log" >&2
        exit 1
    fi
    echo "started (pid $(cat "$PIDFILE"))"
    ;;
stop)
    if pid="$(running_pid)" && [ -n "$pid" ]; then
        kill "$pid"
        rm -f "$PIDFILE"
        echo "stopped"
    else
        rm -f "$PIDFILE"
        echo "not running"
    fi
    ;;
status)
    if running_pid >/dev/null; then
        echo "running (pid $(cat "$PIDFILE"))"
    else
        echo "not running"
        exit 1
    fi
    ;;
*)
    echo "usage: $0 start|stop|status" >&2
    exit 2
    ;;
esac
