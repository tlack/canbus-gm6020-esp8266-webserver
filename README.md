# GM6020-CANBUS-WebServer (ESP8266)

This is an example Arduino Sketch that presents a webserver controlling a CANBUS device.

Specifically:
- a DJI Robomaster GM6020 motor,
- on a Wemos D1 Mini ESP8266,
- via the Seeed Studio CAN-BUS shield v1.2

## Features

- Just a demo/exploration. Not active.
- HTTP API

## Setup

Connect ESP8266 to CAN-BUS shield via SPI.

Important: Edit source to match your CS pin to your configuration. It's typically D8 on
this board but I couldn't use that for some reason.

Connect CAN-H/CAN-L to 2pin CAN cable from GM6020.

Important: Edit source's MOTOR_ID to match GM6020 dip switch ID. Don't forget
to turn on termination.

Connect power to GM6020.

Connect 2pin CAN cable to GM6020.

Important: Edit this software to reflect your wireless access point settings.

Connect ESP8266 to your computer.

Set serial/COM port appropriately.

Flash this software on your ESP8266.

Start Arduino Serial Monitor. Use 115,200 as the baud rate.

Observe ESP8266 serial output. It should look something like this:

```
starting wifi..
wifi up: 192.168.1.212
mdns up: canbot0
starting canbus..
Enter setting mode success 
set rate success!!
Enter Normal Mode Success!!
canbus up
http up: http://192.168.1.212
```

Visit this URL in your browser for a fairly uninteresting status display.

Turn the motor and watch the "p" value change appropriately.

## HTTP API

To use this as part of a large software assembly, use its API

```
$ curl http://192.168.1.212/get
{"ga":0,"p":0,"r":0,"a":0,"t":0}
```

`ga` - goal angle - range 0..8192
`p` - current spindle position - range 0..8192
`r` - current rpms - range 0..360 ? 
`a` - current amps - range 0.. ?
`t` - current temp - range 0..160

The motor will attempt to match a position given. This presently doesn't work
too well - it stops early or overshoots.  You could add a PID controller to the
loop. I am investigating a reinforcement learning approach.

Here's how you use the goal function:

```
$ curl http://192.168.1.212/set?angle=8181
{"gp":8181}
```

## Help

This software is basically unsupported, but feel free to open an issue, or hit
me up on the Telegram ESP32 channel:

https://t.me/esp32hax

