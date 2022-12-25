# Welcome to the Dangar Marine boat monitoring project!

## Requirements

The main goal of this project is to drink wine with our mates. In addition to this we hope to make a hardware device that can be installed into everything from a small tinny with no battery or rain protection to a large motor cruiser with a dry cabin and 1KW of solar. Boats with 1.1KW of solar are currently out of scope.

It is the purpose of the device to protect the boat from harm. For example, by monitoring the boat's battery banks and sending an alert to the owner when the battery gets below a configured voltage you he or she can be sure that the fridge never turns off and the beer gets warm. This can also be useful to ensure that the bilge pump remain operating and the boat doesn't sink.

1. Voltage: The device should have the ability to monitor at least three battery banks. Each monitor circuit voltage divider should have a dip switch to select between 12 or 24 volts.
2. Continuity: Our primary use case for this is bilge alarms. This is a float switch that sits above the bilge. Another use case would be things like magnetic reed switches on doors for a security alarm.
3. Current: This can be used for bilge pumps to monitor how often they are coming on to monitor leaks as well as seeing if the bilge pump is failing or blocked up with debris (current draw will go up with the extra resistance of a blockage to the point where a complete blockage will often blow the fuse). I can see the need to monitor ten or more circuits for current draw to better manage things like networking, refridgeration, AIS etc that are always on. Current monitoring can also be used to monitor current coming is such as from a solar array.
4. Geofencing: Boats occasionaly disappear. Sometimes through theft, but mostly through dragged anchors and broken mooring lines. A notification should be sent when the boat moves out of a defined radius.

If a boat doesn't have a battery then it can't have a bilge pump. This essentially rules out 1. and 2. leaving on geofencing. This is a highly unlikely scenario and can most likely be considered out of scope.

It would not hurt for the device to have it's own battery that charges from the boat's battery, however, to allow it to continue to send alert notifications after the boat's battery has gone flat. 

## Networking

The device should monitor all of the configured voltages, currents and continuities and send them to either a server or point-to-point device. 

1. Point-to-point has the advantages of working anywhere in the world, a rare requirement admittedly.
2. A server allows multiple people to subscribe to the data stream/ alerts and can be viewed and received on any computer or personal device.

There are two main classes of data the system must handle.
1. General monitoring: This is historical data of battery voltages, boat location, bilge pump duty cycle etc.
2. Alerts: These are push notifications when the battery voltage drops below a certain level, the bilge pump stays on for more than a certain amount of time or the boat leaves its defined geofence.

Regardless of how the notifications are delivered, they should all be sent to the owner's phone as an alert.

There are many mechanism for transmitting data from the device to the server. These include:

1. WIFI - Perhaps connected to a standard LTE modem. - https://wisemarket.com.au/collection/accessories/internet-devices/telstra-4gx-usb-modem
2. Particle Device Cloud - https://docs.particle.io/boron/
3. LTE/GSM - No idea!
4. LoRa - https://core-electronics.com.au/rfm69hcw-wireless-transceiver-915mhz.html
5. SigFox - https://thinxtra.com/iot-connectivity/access-station-micro/

If LTE is used to send the notifications point-to-point to the user, this can either be through SMS messages or IP based services such as Pushover and Twilio. Pushover and its ilk use and app where Twilio will send an SMS to the user. All three methods have various advantages and disadvantages:

1. Raw SMS: Requires wiring an LTE modem and learning how to send SMS messages directly. The advantage is no other intermediary service is requried.
2. Internet SMS: Requires wiring and configuring an internet gateway. The advantage of systems like Twilio is that it will work with any form of access to the internet.
3. Internet-based messaging system: Requires wiring and configuring an internet gateway. The advantage of systems like Pushover is that they provide extra functionality such as message priorities, grouping by apps etc.

## Hardware

The hardware to impelment this system on is most likely ESP32, but any microcontroller that can be powered by a lithium battery and a small solar cell would be suitable. ESP32 is favoured due to excellent sensor library support in the form of SensESP (https://github.com/SignalK/SensESP) and build in WIFI. The Sailor Hat ESP32 board ([SH-ESP32](https://hatlabs.fi/product/sailor-hat-with-esp32/)) also has 8-26V input and CAN-BUS (NMEA) support making it highly appealing.

The hardware module to use to provide network connectivity is unknown. Current development is around sending messages over an internet connection in a WIFI-connected lab environment.

## Software

Signal K - https://signalk.org/ Signal K is a standard for data sharing in a marine environment. It provides a gateway between NMEA data and a PC. Signal K is exceptional for large boats that can have a permenantly-powerd Raspberry Pi but not practically for small boats such as commuter tinnys.

node-red - https://www.youtube.com/watch?v=GeN7g4bdHiM A great piece of software for designing flows of data leading to alerts, transformations of visual dashboards of data.

MQTT - https://docs.arduino.cc/tutorials/uno-wifi-rev2/uno-wifi-r2-mqtt-device-to-device

## Victron Connect 

https://pysselilivet.blogspot.com/2021/02/victron-vedirect-with-raspberry.html

## Other Hardware

4G Modem: https://www.mwave.com.au/product/teltonika-rut240-industrial-4g-lte-wireless-router-with-wifi-ac21176

RF Comms: https://jfrog.com/connect/post/nrf24-vs-lora-for-wireless-communication-between-iot-devices/

DFR0571 DC-DC power module: https://core-electronics.com.au/dc-dc-buck-mode-power-module-8-28v-to-5v-3a.html

SEN0291 watt meter: https://core-electronics.com.au/gravity-i2c-digital-wattmeter.html

## Resources

Using the Arduino IDE with ESP32 boards - https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/

Measuring voltage with an Arduino and a voltage divider - https://startingelectronics.org/articles/arduino/measuring-voltage-with-arduino/

This project is based on the template for [SensESP](https://github.com/SignalK/SensESP/) projects.

Comprehensive documentation for SensESP, including how to get started with your own project, is available at the [SensESP documentation site](https://signalk.org/SensESP/).
