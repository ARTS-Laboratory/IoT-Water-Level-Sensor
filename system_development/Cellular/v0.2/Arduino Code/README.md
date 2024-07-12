# Arduino Code

## Sensor Code
The INO file is the Arduino code which can be uploaded to an Arduino Mega. Remember to include
the Adafruit user name and AIO key so that it will connect to the right server.

## Special Libraries
The code will not work without special Arduino libraries that have been altered 
and need to manually installed in the "Arduino libraries" folder, overwriting the existing libraries if they
were already installed. All files are tested and are mutually compatible. We have found compatibility
issues with other versions of these libraries, so it is important that all are manually installed
in the Arduino libraries folder.

## Auxillary Code
This contains three helpful Arduino codes. One sets the time on an RTC in order to calibrate it. 
The other is a demo code which can also be found in the Botletics folder in the Special Arduino Libraries folder.
The LTE Demo is useful to troubleshoot connectivity issues with the SIM card. The last is an I2C address finder which
can be used to find the I2C address of the INA219 current measuring chip.