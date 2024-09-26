//Based on code from Adafruit - www.Adafruit.com
//Credit given to all creators of included libraries
//This project is open source
//Remote Cellular Water Height Sensor developed by Nicholas Liger, from previous work by Corrine Smith and Winford Janvrin
//in conjuction with ARTS - LAB at University of South and Carolina and South Carolina Department of Environmental Services (SC DES)

/************************* All Included Libraries - Do not change this section! *********************************/
//The below three libraries allow the Arduino to connect to the Adafruit IO
#include "Adafruit_MQTT.h" //Adafruit Arduino MQTT library
#include "Adafruit_MQTT_Client.h" //Adafruit Arduino MQTT library
#include <WiFi101.h> //Arduino Wifi library 
//The below two libraries work the SD reader as well as the SPI for communicating with the SD card reader
#include <SPI.h>  //SPI communication library
#include <SD.h> //SD card library
//The below three libraries work the INA219 chip, RTC, and the I2C protocol for communicating with them
#include <Wire.h> //library for using I2C communication
#include <Adafruit_INA219.h> //Adafruit library for controlling INA219 voltage/current sensing chip
#include <DS3231.h> //library for RTC module
#include <LowPower.h> //library for using the sleep mode watchdog timer
#include "NewPing.h" //New Ping library for use with the sonar sensors

/************************* WiFI Setup *****************************/
char ssid[] = "";     //  your network SSID (name)
char pass[] = "";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;    // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS; //Wifi variable, do not edit

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com" //Adafruit IO server information
#define AIO_SERVERPORT  1883 //Adafruit IO server information
#define AIO_USERNAME    "" //Username of Adafruit account you are trying to connect with
#define AIO_KEY         "" //Key for Adafruit account you are trying to connect with

/************Setting up Wifi - Don't change anything in this section! ******************/
//Set up the wifi client
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
#define halt(s) { Serial.println(F( s )); while(1);  }

/****************************** Creating Feeds for the Adafruit IO***************************************/
// MQTT paths for Adafruit IO should follow the form: <username>/feeds/<feedname>
// After Publish/Subscribe, the name of the feed is saved as a variable for later use!
/******************************Feeds for the Adafruit IO***************************************/
//All Publish paths are for sending data from the Arduino to the Adafruit Server
Adafruit_MQTT_Publish sonar1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sonar-sensor-1");
Adafruit_MQTT_Publish sonar2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sonar-sensor-2");
Adafruit_MQTT_Publish batteryVoltage = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/battery-voltage");
Adafruit_MQTT_Publish internalTemperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish signalStrength = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/signal-strength");
//All Subscribe paths are for sending data from the Adafruit server to the Arduino
Adafruit_MQTT_Subscribe initialHeight1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/initial-height-1");
Adafruit_MQTT_Subscribe initialHeight2 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/initial-height-2");

/****************************** Declaring Other Variables ***************************************/
//Variables for setting up the INA219 power monitoring device
Adafruit_INA219 ina219; //declares the INA219 variable, do not edit
float busVoltage = 0; //the voltage reading on the battery input from INA219

//Variables for setting up the two ultrasonic sensors
const int trigPin1 = 47; //trigger pin for sonar sensor 1
const int echoPin1 = 46; //echo pin for sonar sensor 1
const int trigPin2 = 45; //trigger pin for sonar sensor 2
const int echoPin2 = 44; //echo pin for sonar sensor 2
const int maxDist = 600; // max distance in cm for sonar sensor, same for both
NewPing sonar_sensor_1(trigPin1, echoPin1, maxDist); //set up NewPing variable for sonar sensor 1
NewPing sonar_sensor_2(trigPin2, echoPin2, maxDist); //set up NewPing variable for sonar sensor 2
float distance1=0; //variable for distance measured by sonar sensor 1
float distance2=0; //variable for distance measured by sonar sensor 2
float initHght1 = 0; //Elevation of the ultrasonic sensor 1 at set up, used
//to calibrate the sensor readings (usually done in reference to sea level)
float initHght2 = 0; //Elevation of the ultrasonic sensor 1 at set up, used
//to calibrate the sensor readings (usually done in reference to sea level)
float height1 = 0; //Height variable for the current water height calculated from ultrasonic sensor 1
//(usually done with reference to sea level)
float height2 = 0; //Same for sensor 2
int failCount = 0; //used to track failures to stop infinite loops

//Variables for setting up the SD card and RTC
char timeStamp[32]; //Char/string variable for holding time stamp
const int chipSelect = 4; //Chip Select for the SD card on the WINC1500 shield
char dayStampFileName[20]; //Char/string variable for holding the day stamp for naming
//file in SD card
DS3231 myRTC; //RTC variable for using DS3231 library scripts

//Variables governing the sampling rate of the sensor
uint8_t currentTime = 1; //variable used to hold "minute" reading of RTC
uint8_t lastTime = 1; //variable used to hold the previous "minute" reading of RTC
//See code later for more description of the above variables
int delayTime = 5; //time in minutes between between readings

#define MQTT_CONN_KEEPALIVE 300 //Adafruit IO defaults to 5 mins of connection
bool firstRun = true;

//*******************************Void Set up************************************
void setup() {
  Serial.begin(9600); //begin the Serial monitor
  Wire.begin(); //begin communication with I2C, needed for RTC module and INA219 chip

  //The below section activates the SD card
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  while (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present. Retrying...");
    delay(2000);
  }
  Serial.println("card initialized.");

  //These following lines start up and calibrate the INA219 chip
   while (!ina219.begin()) { //these lines initialize the INA219 chip
    Serial.println("Failed to find INA219 chip. Retrying...");
    delay(2000);
  }

  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  ina219.setCalibration_16V_400mA();


  // Initialise the Wifi Client to connect to the Internet
  Serial.print(F("\nInit the WiFi module..."));
  // check for the presence of the breakout
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WINC1500 not present");
    // don't continue:
    while (true);
  }
  Serial.println("ATWINC OK!");

  //These lines set up the ability to subscribe from the Adafruit IO Dashboard
  mqtt.subscribe(&initialHeight1);
  mqtt.subscribe(&initialHeight2);
  MQTT_connect(); //This connects our sensor package to the Adafruit IO
}

void loop() {
  Serial.println("The loop begins here!");
  //This if statement will not allow the program to proceed until the Initial Height for both ultrasonic sensors have been
  //entered from the Adafruit IO
  while (initHght1 == 0 || initHght2 == 0){
      // This is our 'wait for incoming subscription packets' busy subloop
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(5000))) {
      if (subscription == &initialHeight1) {
        Serial.print(F("*** Got: "));
        Serial.println((char *)initialHeight1.lastread);
        initHght1 = atof((char *)initialHeight1.lastread);
      }
      if (subscription == &initialHeight2) {
        Serial.print(F("*** Got: "));
        Serial.println((char *)initialHeight2.lastread);
        initHght2 = atof((char *)initialHeight2.lastread);
      }
    }
    Serial.println(initHght1);
    Serial.println(initHght2);
    Serial.println("You have not entered the initial heights yet!");
    delay(2000);
    }
  DateTime now = RTClib::now();; //creates the "now" variable from the RTC to save the timeStamp
  currentTime = now.minute(); //saves the current minute as a variable

  //This "if" loop contains all of the code to run the package on loop. The sampling rate set in the Adafruit Dashboard gives the
  //frequency this loop runs on. Every nth minute, this loop will transmit data to the Adafruit IO server
  if (currentTime %delayTime == 0 && currentTime != lastTime || firstRun == true){
    //We first run the MQTT connect function to make sure that our connection to the server is active! 
    //If not, then the Wifi will attempt to reconnect. See details below
    MQTT_connect();
    //This section creates the timestamps from the RTC module
    sprintf(timeStamp, "%02d:%02d:%02d %02d/%02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());  
    //the above line creates a timestamp that is used to date data saved to the SD card
    snprintf(dayStampFileName, 20, "%02d%02d%02d.txt", now.day(), now.month(), now.year());  
    //the above line creates a name for the SD card files so that a new file is created every new day
    Serial.print(F("Date/Time: ")); //uncomment to debug
    Serial.println(timeStamp);

    //This line retrieves temperature in Celcius from the RTC
    float rtcTemp = myRTC.getTemperature();
    float tempF = rtcTemp*(1.8)+32; //convert celcius to farenheit
    Serial.println(tempF);

    //This line gets the voltage of the battery from the INA219 chip
    busVoltage = ina219.getBusVoltage_V()+0.8; //get voltage from battery and add 0.8V to account for drop across diode
    Serial.println(busVoltage);

    //This section gets the current distance from the sonar sensors and calculates
    //the current height based on the elevation of sensor minus the distance
    // h = h0 - d, and converts the reading to ft
    distance1 = sonar_sensor_1.convert_in(sonar_sensor_1.ping_median(5)); //The median function gives the median value of five quick samples
    delay(1000); //This second delay keeps the sensors from interfering with each other
    distance2 = sonar_sensor_2.convert_in(sonar_sensor_2.ping_median(5)); //same as above for sensor 2

    //To further reduce misreads, check if distance is equal to zero
    //If the sensor times out, then the Arduino will record the distance
    //inaccurately as zero, and the water surface elevation will appear to
    //be equal to the installation height, so we check here if distance1 or distance2
    //equals zero, and if so, we try again until we get a non-zero answer
    failCount = 0; //reset the failure counter before going into the while loop to stop an infinite loop
    //if failCount exceeds ten, then Arduino moves on to prevent all data for the run being lost
    while (distance1 == 0 && failCount <10){
      Serial.println("Distance 1 is reading zero");
      distance1 = sonar_sensor_1.convert_in(sonar_sensor_1.ping_median(5)); //The median function gives the median value of five quick samples 
      delay(1000);
      failCount++;
    }
    failCount = 0; //reset the failure counter before going into the while loop to stop an infinite loop
    while (distance2 == 0 && failCount <10){
      Serial.println("Distance 2 is reading zero");
      distance2 = sonar_sensor_2.convert_in(sonar_sensor_2.ping_median(5)); //The median function gives the median value of five quick samples 
      delay(1000);
      failCount++;
    }
    height1 = initHght1 - (distance1)/(12); //convert from inches to feet
    height2 = initHght2 - (distance2)/(12);
    Serial.print("Height 1 is: "); //uncomment to debug
    Serial.print(height1);
    Serial.print("  Height 2 is: ");
    Serial.println(height2);

    // This section saves the data to the SD card, before we attempt to transmit the data over the cell network
    //Because the cell connection crashes frequently, we save the data to the SD card first so that we can have
    //a more complete set of data for future investigation
    Serial.println(dayStampFileName);
    //This line creates a new file named with the day time stamp (DDMMYYYY)
    //or opens the file if this name already exists
    //The SD cards have a tendency to become corrupted which can ruin a long term test
    //if the data is all saved to a single file, so this method creates a new CSV for every day
    //so that even if a file is corrupted, we will still have the data from every other day
    if (SD.exists(dayStampFileName) == 0) {
      File dataFile = SD.open(dayStampFileName, FILE_WRITE);
      if (dataFile){
        dataFile.println("Ultrasonic 1 Height Reading (ft),Ultrasonic 2 Height Reading (ft),Battery Voltage (V),Internal Temperature (F),Timestamp");
        dataFile.close();
      }
      else {
      Serial.println("error opening datalog.txt");
      }
    }
    // The below section creates a new entry in SD card which records all the relevant
    // information and the timestamp in a CSV format
    File dataFile = SD.open(dayStampFileName, FILE_WRITE);
    if (dataFile) {
        dataFile.print(height1);
        dataFile.print(",");
        dataFile.print(height2);
        dataFile.print(",");
        dataFile.print(busVoltage);
        dataFile.print(",");
        dataFile.print(tempF);
        dataFile.print(",");
        dataFile.println(timeStamp);
        dataFile.close();
      }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    }

  MQTT_connect(); //Recheck the connection to Wifi
  //This line checks the Wifi signal in Raw signal strength (RSSI)
  float n = WiFi.RSSI();
  Serial.print(n); Serial.println(F(" RSSI"));

  // Now publish all the data to different feeds via the Adafruit server!
  if (distance1 != 0){
    if (! sonar1.publish(height1)) {
      Serial.println(F("Failed"));
    }
  }
  if (distance2 != 0){
    if (! sonar2.publish(height2)) {
      Serial.println(F("Failed"));
    }
  }
  if (! batteryVoltage.publish(busVoltage)) {
     Serial.println(F("Failed"));
   }
    if (! internalTemperature.publish(tempF)) {
     Serial.println(F("Failed"));
   }
    if (! signalStrength.publish(n)) {
     Serial.println(F("Failed"));
   }
  // Delay until next post
    lastTime = currentTime; //This prevents the "if" loop from running again and again
    //in the same minute

    //Serial.println((char *)initialHeight.lastread); //uncomment to debug
    Serial.println("Going to sleep!");
    firstRun = false;
  } 
  //Between sampling times, the Arduino is sent into light sleep which helps to save on power
  //It wakes up every eight seconds to check the time and goes back to sleep unless the sampling rate
  //conditions are met in the "if" loop
  else {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    delay(5);
  }
}  

//Our custom code ends here. Everything below are premade function codes

//************************** Function Codes - Do not alter these! **************************
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
/************************** Connecting/Reconnecting Check to the Adafruit IO Server - Do not change! ************************************/
void MQTT_connect() {
  failCount=0; //reset the fail counter
  int8_t ret;

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }
  
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0 && failCount<10) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       failCount++;
  }
  if (failCount == 10){
    Serial.println("The Wifi could not connect currently.");
  }
  Serial.println("MQTT Connected!");
}