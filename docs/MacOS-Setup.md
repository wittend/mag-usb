# macOS Setup

This guide helps you install, build, and run mag-usb on a macOS computer.

## Prerequisites
- macOS host with USB 2.0 support
- Docker Desktop
- socat (install with Homebrew: `brew install socat`)
- Optional: Microsoft VS Code, VSCodium, or JetBrains CLion for an IDE workflow
- Pololu Isolated USB-to-I²C Adapter (products 5396 or 5397)
- RM3100-based magnetometer board

## Hardware connection
1. Connect the Pololu USB-to-I²C adapter to your host via USB.
2. Wire SDA/SCL/GND (and 5V if required) between the adapter and the sensor board.
3. Verify the adapter name in macOS by running `ls -l /dev/cu.*` before and after
   plugging in the adapter. On macOS, the adapter typically appears as
   /dev/cu.usbmodem14#01.

## Adapter Communication
After installing socat, run the following command in a separate terminal, substituting
`<DEVICE>` for the name of the adapter as verified during hardware setup, and `<PORT>`
for the port number the device should listen to:

```bash
socat -d -d TCP-LISTEN:<PORT>,reuseaddr,fork FILE:<DEVICE>,raw,echo=0
```

You should see a similar output:

```
2026/06/04 14:43:09 socat[69640] N listening on LEN=16 AF=2 0.0.0.0:<PORT>
```

Example usage with device /dev/cu.usbmodem14101 and port 1234:

```bash
socat -d -d TCP-LISTEN:1234,reuseaddr,fork FILE:/dev/cu.usbmodem14101,raw,echo=0
```

Keep note of what port you have the adapter assigned to. Docker will use this port to
communicate with the adapter.

## Deploying mag-usb
Open Docker Desktop to ensure the Docker daemon is running. In another terminal,
navigate to the project directory and execute the following command:

```docker
docker compose run --rm host
```

This will create a Docker container with mag-usb already built. Upon launching the
container, the green LED on your Pololu adapter should blink once along with the
following output being displayed in your terminal:

```
(1) Starting virtual TTY bridge...
(2) Waiting for virtual device...
READY!
   To edit config: nano config.toml
   To run mag-usb: ./mag-usb
```

The terminal running socat should display an output similar to the example below:

```
2026/06/04 21:40:18 socat[40636] N accepting connection from LEN=16 AF=2 127.0.0.1:52866 on LEN=16 AF=2 127.0.0.1:1234
2026/06/04 21:40:18 socat[40636] N forked off child process 49661
2026/06/04 21:40:18 socat[40636] N listening on LEN=16 AF=2 0.0.0.0:1234
2026/06/04 21:40:18 socat[49661] N opening character device "/dev/cu.usbmodem14101" for reading and writing
2026/06/04 21:40:18 socat[49661] N starting data transfer loop with FDs [6,6] and [5,5]
```

Refer to the [troubleshooting](#troubleshooting) section at the end of this page if the output in either
terminal is different.

By default, the container binds to port 1234. If the port you have the adapter
assigned to in socat is different, you must override the environment variable:

```docker
docker compose run --rm -e HW_PORT=4321 host
```

### Using WebSockets
For WebSocket broadcasting, the container broadcasts on and exposes port 8765 by
default. To change the port number, just publish a different port in the command:

```docker
docker compose run --rm -p 8443:8443 host
```

The config file must be edited inside the container to reflect any changes:

```bash
nano /etc/mag-usb/config.toml
```

Then, in nano:

```toml
[websocket]
# Enable WebSocket output server.
enable = true
# Bind address and port for WebSocket clients.
bind_address = "0.0.0.0"
port = 8443 # Change to port published in docker compose
```

## Troubleshooting

### Refused Connection
If during the execution of the container you see a similar output:

```
(1) Starting virtual TTY bridge...
(2) Waiting for virtual device...
2026/06/05 00:25:33 socat[7] E connect(7, AF=2 192.168.65.254:1234, 16): Connection refused
```

Exit the container with ctrl+c. Check the adapter to make sure it is properly connected
to your computer by unplugging and plugging it back in.

Verify that the device name is correct by running `ls -l /dev/cu.*` after unplugging
and plugging the adapter into your computer.

Check socat to verify that it is running and what port the device is listening to:

```bash
socat -d -d TCP-LISTEN:<PORT>,reuseaddr,fork FILE:<DEVICE>,raw,echo=0
```

Explicitly set the port during execution of the container. The value for `<PORT>` must
be identical in both socat and docker compose:

```docker
docker compose run --rm -e HW_PORT=<PORT> host
```

### Device Unavailable
If upon executing mag-usb you see this error:

```
I2C adapter device '/dev/ttyACM0' not available (error -2). Exiting...
```

This error is a result of socat exiting or the physical device being disconnected. The
container must be exited as it cannot reconnect to the adapter. Check the physical
adapter to make sure it is plugged in and restart socat.

If the device in the error is anything other than /dev/ttyACM0, check your config.toml
file for this line under `[i2c]` to make sure it matches:

```toml
portpath = "/dev/ttyACM0"
```

The container only reads from /dev/ttyACM0. Any other device will cause mag-usb to exit.