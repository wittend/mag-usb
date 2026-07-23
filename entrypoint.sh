#!/usr/bin/env bash
set -e

echo '-------------------------------------------'
echo '(1) Starting virtual TTY bridge...'
socat PTY,link=/dev/ttyACM0,raw,echo=0 TCP:host.docker.internal:${HW_PORT} &

echo '(2) Waiting for virtual device...'
while [ ! -e /dev/ttyACM0 ]; do
    sleep 0.5
done

echo 'READY!'
echo '   To edit config: nano /etc/mag-usb/config.toml'
echo '   To run mag-usb: ./mag-usb'
echo '-------------------------------------------'

exec bash
