# espidf-playground

## Setup
- git clone https://github.com/mohankumargupta/espidf-playground.git
- cd espidf-playground
- make menuconfig -> Serial flasher config -> Default serial port
-                 -> Component config -> FAT filesystem support -> Long filenames on Stack
- make flash monitor

## Components
- wifi-ap
- http_server
- websockets_server

### wifi-ap
Default SSID and password are found [here](https://github.com/mohankumargupta/espidf-playground/blob/master/components/wifi_ap/include/wifi_ap.h). Default IP Address is 192.168.4.1
  
### http_server
Running on port 80.

Endpoints:
  - /
  - /boo
  - any other path will be searched in SD card

SD card support (tested in SPI mode):
![esp32 sdcard](https://camo.githubusercontent.com/fe6b89251ae4df2628b1a4c86c57976f22d6d5ba/687474703a2f2f692e696d6775722e636f6d2f34436f584f75522e706e67) 

Didn't need pullups or jumpers for GPIO0 and GPIO2, just using standard SPI pins. 
- Tested with files [here](https://github.com/mohankumargupta/espidf-playground/blob/master/test_data/httpserver_sdcard) placed on SD card

### websockets_server
Running on port 8080.

- websockets reference [here](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers)

To test, I used [test.html](https://github.com/mohankumargupta/espidf-playground/blob/master/test_data/websockets/test.html)

(Not served, just opened locally on PC)

# TODO
- change SD card pinouts back to default as the SD Card example
- reduce heap footprint




