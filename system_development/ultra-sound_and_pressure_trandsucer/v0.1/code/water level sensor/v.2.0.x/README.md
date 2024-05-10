# WHS_v2.0 (STABLE)
- compatible with datalogging shield
- AT+CSCLK=1, sleep mode controlled by DTR pin
- copied over, formerly v2.0.1
- Sampling rate does not exceed 5 minutes
- any connectivity issues experienced should be from poor environment or network provider's side
- Publishing feeds: location, ultrasonic stage, pressure stage, temperature, ambient pressure, update gps
- Subscribing feeds: deployment toggle, sampling rate, initial elevation, update gps

# WHS_v2.0.1.ino
- added fona lipo feed

# WHS_v2.0.2.ino
- allow maximum delay to exceed 5 minutes

# WHS_v2.0.3.ino
- add external temperature sensor

# WHS_v2.0.4.ino
- add voltage sensor and integrate with power board

# WHS_v2.0.5.ino
- add median filtering technique

# WHS_v2.0.6.ino
- copied from WHS_v2.0.4
- does not have the median filter in order to send raw data and post process
- add contact sensor feed