HS110(AU) HW 2.0 Client
=======
## Version 1.2

An api for controlling the TP-Link HS110 Smartplug written in C.
Supports both linux and windows.

To compile with Windows use MinGW with -lws2_32.

Usage: In command prompt/terminal type (FilePath) -i (IPV4 address) (command)
Example: ./HS110Client -i 192.168.0.2  -c on 


List of commands:
* on: Turn on device 
* off: Turn off device
* reboot: Reboot device
* info: Retrieve system info including MAC, device ID, hardware ID etc
* emeter: Retrieve real time current and voltage readings 
* gain: Retrieve EMeter VGain and IGain settings
* LEDoff: Turn off LED light 
* LEDon: Turn on LED light 

Options:

* --udp: send command via UDP instead of TCP (allows broadcasting with ip set to 255.255.255.255)


