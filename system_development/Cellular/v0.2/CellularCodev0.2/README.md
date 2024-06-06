# The Arduino code for running the water height sensor.

The INO file is the Arduino code which can be uploaded to an Arduino Mega. Remember to include
the Adafruit user name and AIO key so that it will connect to the right server.

The code will not work without special Arduino libraries that have been altered 
and need to manually installed. The Arduino libraries are in the marked folder and must
be manually installed in the "Arduino libraries" folder, overwriting the existing libraries if they
were already installed. All three files are tested and are mutually compatible. We have found compatibility
issues with other versions of these libraries, so it is important that all three are manually installed
in the Arduino libraries folder.

The auxillary code contains two helpful Arduino codes, one the set the time on an RTC in order to calibrate it. 
The other is a demo code which can also be found in the Botletics folder in the Special Arduino Libraries folder.
The LTE Demo is useful to troubleshoot connectivity issues with the SIM card.