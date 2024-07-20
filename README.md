# IoT Water Level Sensor

This is a water height sensor developed by the ARTS-Lab at the University of South Carolina in conjunction with the South Carolina Department of Environmental Services (SC DES) Dam Safety Program. The purpose of this sensor is to provide a low-cost, open-source alternative to traditional gaging equipment. 

<p align="center">
<img src="media/Lake Wallace Installation 2.png" alt="drawing" width="600"/>
</p>
<p align="center">
Sensor deployed above inlet to Lake Wallace in Bennetsville, SC.
</p>
<p>
</p>

<p align="center">
<img src="media/Adafruit GUI.png" alt="drawing" width="600"/>
</p>
<p align="center">
The online GUI using Adafruit that displays water elevation in real time.
</p>

## Features
### Water Elevation Measurement
The sensor package uses two ultrasonic sensors to measurement the height of water in a reservoir, lake, or pond. These sensors are low cost, but are accurate to +/- 1 inch within a range of 18 ft.
<p align="center">
<img src="media/Ultrasonic Sensor.png" alt="drawing" width="400"/>
</p>
<p align="center">
The low cost, JSN-SR04T ultrasonic sensor used with Arduinos.
</p>

### Internet Connectivity
Our package can connect to the Adafruit online server. A free account can be made with them to host data online, and a small subscription fee can upgrade the service for more data storage.
</p>
This device can connect to Internet via cellular or Wifi.
<p align="left">
<img src="media/Cellular Board.png" alt="drawing" width="300"/>
<align="right">
<img src="media/Wifi Board.png" alt="drawing" width="300"/>
</p>
On the left, the SIM7000 cellular shield. On the right, the WINC1500 Wifi shield.
</p>

### Long Term Data Storage
Because the online server only saves data for 60 days, to ensure that all data is recorded for future use, we use an SD card and Real Time clock with this sensor package to save all data with a timestamp in a .CSV format. 

## System Development
Contains the hardware and software developed for the project. Use this folder to create and troubleshoot a sensor.

## Licensing and Citation

This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License [cc-by-sa 4.0].

[![License: CC BY-SA 4.0](https://img.shields.io/badge/License-CC_BY--SA_4.0-lightgrey.svg)](https://creativecommons.org/licenses/by-sa/4.0/)


Cite this as: 

@Misc{ARTSLabIotWaterLevel,    
  author = {ARTS-Lab},  
  howpublished = {GitHub},  
  title  = {IoT Water Level Sensor},   
  groups = {ARTS-Lab},    
  url    = {https://github.com/ARTS-Laboratory/IoT-Water-Level-Sensor},   
}


