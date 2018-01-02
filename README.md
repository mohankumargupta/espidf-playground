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

SD card support: 
- Tested with files [here](https://github.com/mohankumargupta/espidf-playground/blob/master/test_data/httpserver_sdcard) placed on SD card

### websockets_server
Running on port 8080.

To test, I used [test.html](https://github.com/mohankumargupta/espidf-playground/blob/master/test_data/websockets/test.html)

(Not served, just opened locally on PC)

# TODO
- http_server: handle large files 
- websockets: handle payload larger than 127 bytes
- reduce heap footprint




