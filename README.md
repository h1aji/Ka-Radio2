# ESP8266 Web Radio

Karadio is a WiFi shoutcast player based on esp8266 and vs1053b chips
This repo is a fork of [Ka-Radio](https://github.com/karawin/Ka-Radio) which is compiled with [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos)

Please note: project is currently under developement

### Installation

Karadio requires [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos). Installation guide can be found on the main page.

Install python modules required for web interface build

```sh
$ sudo pip3 install rjsmin
$ sudo pip3 install rcsmin
```

Clone karadio repository next to esp-open-rtos dir and run make.

```sh
$ cd ~/esp/
$ git clone https://github.com/h1aji/karadio.git
$ cd karadio
$ make
```

Flash new image

```sh
$ make flash -j4
```
* flash options can be edited in local.mk file

### Wiring
```
ESP pin   - VS1053 pin
------------------------
GPIO14    - SCK
GPIO12    - MISO
GPIO13    - MOSI
GPIO16    - DCS
GPIO10    - DREQ
          - RST *
GPIO02    - CS

*=connected through diodes
```

Optional external RAM 23LCV1024 supported

```
ESP pin   - 23LC1024 pin
------------------------
GPIO15    - CS   (1)
GPIO12    - MISO (2)
          - SIO2 (3) *
GND       - GND  (4)
GPIO13    - MOSI (5)
GPIO14    - SCK  (6)
          - SIO3 (7) *
3.3V      - VCC  (8)

*=optional, may also be connected to Vcc on 23LC1024 side.
```

External RAM is detected at boot time if connected.


Infra-red sensor

```
GPIO00  - OUT
```

LCD 2004

```
GPIO04  - SDA
GPIO05  - SCL
```


### Changelog

### Todos

 - Migrate ntp code to sntp
 - Migrate webserver to extras/http
 - Add I2C LCD 20x4 support
 - Add IR support

License
----

ESPRESSIF MIT
