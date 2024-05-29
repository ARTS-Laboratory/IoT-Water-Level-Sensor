The libraries for the cellular model are hodge-podge and subject to problems. The original Adafruit Fona library has to be overwritten
by the special Botletics Adafruit Fona Library that connects code for use with LTE. The Sim7000 works off of LTE data
and the original Adafruit Fona Library does not contain LTE connection code. Also, the line "#define SSL_FONA 1" in the
.h code in the Fona Library needs to be changed to "#define SSL_FONA 0" for the code to work. Copies of working Arduino libraries are contained
in this folder in case an automatic update changes the Arduino libraries and prevents the module from working.