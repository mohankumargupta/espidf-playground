# espidf-playground

## Setup
- git clone this repo
- make menuconfig -> Serial
- make flash monitor

## Components
- wifi-ap
- http_server
- websockets_server

### wifi-ap
Default SSID and password are found [here](https://github.com/mohankumargupta/espidf-playground/blob/master/components/wifi_ap/include/wifi_ap.h). Default IP Address is 192.168.4.1
  
### http_server
Running on port 80.

SD card support (tested in SPI mode):
- Used pinout shown [here](https://camo.githubusercontent.com/fe6b89251ae4df2628b1a4c86c57976f22d6d5ba/687474703a2f2f692e696d6775722e636f6d2f34436f584f75522e706e67). Didn't need pullups or jumpers for GPIO0 and GPIO2, it still works
for me. 
- Tested with files [here](https://github.com/mohankumargupta/espidf-playground/blob/master/test_data/httpserver_sdcard) placed on SD card

### websockets_server
Running on port 8080.

To test, I used [test.html](https://github.com/mohankumargupta/espidf-playground/blob/master/test_data/websockets/test.html)

(Not served, just opened locally on PC)

# TODO
- http_server: handle large files 
- websockets: handle payload larger than 127 bytes
- reduce heap footprint




