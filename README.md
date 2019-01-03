# ESP32-ESP8266-File-Download-Upload-Delete-Stream-and-Directory

SPIFFS Version
Copy sketch to a folder, including CSS.

SD-Card Version
Copy Sketch to a folder, including CSS and Network and Sys_Variables files all to same directory.

For all versions adjust your netwrok credentials accordingly.

~~

ESP (32/8266) File Services in one application, enables management of files on an ESP using SD-Filing System

Using HTTP and an HTML interface to download files from an ESP32/ESP8266

Download all files to a sketch folder.

Edit the Network tab and add in your SSID and PASSWORD, more if you have them.

Choose your IP address, currently it is fixed to 192.168.0.150

You can edit the logical name 'fileserver' to your requirements then access the device with http://fileserver.local but only if your browser has mDNS support otherwise use http://192.168.0.150/

NOTES:The ESP32 is not reliable when using SD Cards, please ensure you know how to connect the SPI bus to the SD-Card if not using an MH-ET Live ESP32 board and a Wemos SD-Card Shield. Although pull-ups are enabled, you may need to add an external 4k7 pull-up too on the MISO line
