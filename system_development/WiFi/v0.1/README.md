# Wifi V0.1

## WifiCode_v0.1
This folder contains all of the Arduino code used in this sensor project.

## Data Collection - Logging PCB
This folder contains the KiCad files used to create the custom data logging PCB

## Power PCB
This folder contains the KiCad files used to create the custom Power supply and monitoring PCB


## Parts List
A detailed parts list with part numbers and ordering information can be found in the Wifi Water Height Sensor
Package Parts List.

# Important Information:

### WINC1500 Shield

This sensor package uses a WINC1500 shield to connect to the Internet over Wifi.
It is produced by Adafruit Industries. Here is a [tutorial](https://learn.adafruit.com/adafruit-winc1500-wifi-shield-for-arduino) by them for how to set up and use the board.

### Adafruit IO

We use the Adafruit IO server to monitor our sensor package over the Internet in real time. You will need to create an account with Adafruit, however it is free.
To learn how to sign up for the Adafruit IO server, use this [tutorial](https://learn.adafruit.com/welcome-to-adafruit-io).
You will also need to learn how to use [feeds](https://learn.adafruit.com/adafruit-io-basics-feeds) and [dashboards](https://learn.adafruit.com/adafruit-io-basics-dashboards).

### PCB's

To use this sensor, you will have to assemble custom PCB's. All the schematics can be found in the Datalogging and Power PCB folders and parts list. 
We have ordered PCB's from [OSH Park](https://oshpark.com/) and have liked the results. Simply drag and drop the .kicad_pcb file from the PCB folder into OSH Park to create a PCB!

# Assembly Instructions

### Assemble PCB's

The first step is acquire PCB and solder the components. Refer to the .kicad_sch and .kicad_pcb files in the respective folders for the placement of each component. 
Their locations have been labeled on the PCB's for greater ease.

(Include pictures of before and after)

### Fit Boards

The next step is to put together the Arduino Mega, WINC1500 shield, and the completed PCB's as shown:

(Include pictures)

### Formatting the Code

To get the WifiCode_v0.1 to work with your particular Adafruit account, we will have to format the code a little.

First acquire your username and key from the Adafruit IO website:

<p align="center">
<img src="https://github.com/ARTS-Laboratory/IoT-Water-Level-Sensor/blob/main/media/Adafruit%20IO%20Key.PNG" alt="drawing" width=400"/>
</p>

Then, you must create feeds so that the key of the feed matches the provided Arduino code. If your key is not the same as the ones pictured, your data will not be transmitted properly.

<p align="center">
<img src="https://github.com/ARTS-Laboratory/IoT-Water-Level-Sensor/blob/main/media/Adafruit%20IO%20Feed%20Names.PNG" alt="drawing" width=400"/>
</p>

Using your Wifi Network name and password, fill in these lines in the code:

<p align="center">
<img src="https://github.com/ARTS-Laboratory/IoT-Water-Level-Sensor/blob/main/media/WifiPasswordArduinoLocation.PNG" alt="drawing" width=600"/>
</p>

Using your Adafruit username and key, fill in this line in the code:

<p align="center">
<img src="https://github.com/ARTS-Laboratory/IoT-Water-Level-Sensor/blob/main/media/WifiArduinoKey%20Location.PNG" alt="drawing" width=600"/>
</p>

Power your device, turn it on, and start transmitting! All relevant schematics are provided for the PCB's, and the Arduino code is annotated with explanations. Feel free to dig into both to tweak the device to your desire.
