#!/usr/bin/env bash
# Checks the local test broker's access rules: who can publish and subscribe to
# which topics, and that anonymous clients are refused.  Needs the broker
# running (broker.sh start) and setup_broker.sh already run.
set -u

. "$(dirname "$0")/env.sh"
. "$MQTT_RUN_DIR/passwords.env"

B=(-h 127.0.0.1 -p "$MQTT_PORT_PLAIN")
fails=0
out_dir="$(mktemp -d)"
trap 'rm -rf "$out_dir"' EXIT

# A subscriber listens on a topic while a publisher sends one message.
# Reports whether the subscriber got it.
# args: sub_user sub_pw sub_topic pub_user pub_pw pub_topic expected(yes|no)
check() {
    local out="$out_dir/out.$RANDOM"
    mosquitto_sub "${B[@]}" -u "$1" -P "$2" -t "$3" -C 1 -W 3 -v >"$out" 2>/dev/null &
    local sub=$!
    sleep 1
    mosquitto_pub "${B[@]}" -u "$4" -P "$5" -t "$6" -m probe 2>/dev/null
    wait "$sub" || true
    local got=no
    grep -q "^$6 probe" "$out" && got=yes
    if [ "$got" = "$7" ]; then
        echo "PASS  $4 -> $6, seen by $1: $got"
    else
        echo "FAIL  $4 -> $6, seen by $1: got $got, expected $7"
        fails=$((fails + 1))
    fi
}

D=hamsci/mag/TEST/data
C=hamsci/mag/TEST/data/command
O=hamsci/mag/TEST/other

check viewer-TEST "$VIEW_PW"  $D station-TEST "$STATION_PW" $D yes   # station sends samples
check probe-TEST  "$PROBE_PW" $O station-TEST "$STATION_PW" $O no    # station is not allowed off its topics
check probe-TEST  "$PROBE_PW" $C cmd-TEST     "$CMD_PW"     $C yes   # command issuer reaches the station's command topic
check probe-TEST  "$PROBE_PW" $C viewer-TEST  "$VIEW_PW"    $C no    # viewer cannot send commands
check probe-TEST  "$PROBE_PW" $D viewer-TEST  "$VIEW_PW"    $D no    # viewer cannot publish samples
check viewer-TEST "$VIEW_PW"  $D cmd-TEST     "$CMD_PW"     $D no    # command issuer cannot publish samples

if mosquitto_pub "${B[@]}" -t $D -m x 2>/dev/null; then
    echo "FAIL  anonymous publish was accepted"
    fails=$((fails + 1))
else
    echo "PASS  anonymous publish refused"
fi

echo "failures: $fails"
exit "$fails"
