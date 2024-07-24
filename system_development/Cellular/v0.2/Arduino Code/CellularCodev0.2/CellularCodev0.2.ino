// Based on code from Timothy Woo (www.botletics.com)  /Github: https://github.com/botletics/SIM7000-LTE-Shield
//Also based on code from Adafruit - www.Adafruit.com
//Credit given to all creators of included libraries
//This project is open source
//Remote Cellular Water Height Sensor developed by Nicholas Liger, from previous work by Corrine Smith and Winford Janvrin
//in conjuction with ARTS - LAB at University of South and Carolina and South Carolina Department of Health and Environmental Control (SCDHEC)

/************************* All Included Libraries - Do not change this section! *********************************/
//The below three libraries are special. See the Readme for more information.
#include "BotleticsSIM7000.h" // Botletics SIM7000 library developed by Timothy Woo
#include "Adafruit_MQTT.h" //Adafruit library for connection via MQTT to the Adafruit IO server
#include "Adafruit_MQTT_FONA.h" //Adafruit library for using cellular data to connect to Adafruit IO server. 
//The below two libraries work the SD reader as well as the SPI for communicating with the SD card reader
#include <SPI.h>  //SPI communication library
#include <SD.h> //SD card library
//The below libraries work the INA219 chip, RTC, and the I2C protocol for communicating with them
#include <Wire.h> //library for using I2C communication
#include <Adafruit_INA219.h> //Adafruit library for controlling INA219 voltage/current sensing chip
#include <DS3231.h> //library for RTC module
#include <LowPower.h> //library for using the sleep mode watchdog timer
#include "NewPing.h" //New Ping library for use with the sonar sensors

/************************* Botletics SIM7000 Set Up Section - Do not Edit! *********************************/
#define SIMCOM_7000 //from Botletics library, determine the model of SIM chip
// For botletics SIM7000 shield
#define BOTLETICS_PWRKEY 6
#define RST 7
//#define DTR 8 // Connect with solder jumper
//#define RI 9 // Need to enable via AT commands
#define TX 10 // Microcontroller RX
#define RX 11 // Microcontroller TX
//#define T_ALERT 12 // Connect with solder jumper

//Set up serial communication
#include <SoftwareSerial.h>
SoftwareSerial modemSS = SoftwareSerial(TX, RX);
SoftwareSerial *modemSerial = &modemSS;

// Use this one for LTE CAT-M/NB-IoT modules (like SIM7000)
// Notice how we don't include the reset pin because it's reserved for emergencies on the LTE module!
Adafruit_FONA_LTE modem = Adafruit_FONA_LTE();

/************************* Adafruit.io Setup - Enter your username and password for Adafruit IO here *********************************/
#define AIO_SERVER      "io.adafruit.com" //Adafruit IO server information
#define AIO_SERVERPORT  1883 //Adafruit IO server information
#define AIO_USERNAME    "" //Username of Adafruit account you are trying to connect with
#define AIO_KEY         "" //Key for Adafruit account you are trying to connect with

// Setup the FONA MQTT class by passing in the modem class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&modem, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// How many transmission failures in a row we're OK with before reset
uint8_t txfailures = 0;  

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
//random variables for running Botletics scripts, do not edit
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
char imei[16] = {0}; // Use this for device ID
uint8_t type;

// Char/String variables for holding the variables
//for transmitting to Adafruit IO
char battBuff[12],dist1Buff[12],dist2Buff[12],tempBuff[12],signalBuff[12];

//Variables for setting up the INA219 power monitoring device
Adafruit_INA219 ina219(0x40); //declares the INA219 variable, do not edit
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

//Variables for setting up the SD card and RTC
char timeStamp[32]; //Char/string variable for holding time stamp
const int chipSelect = 53; //Chip Select for the SD card
char dayStampFileName[20]; //Char/string variable for holding the day stamp for naming
//file in SD card
DS3231 myRTC; //RTC variable for using DS3231 library scripts

//Variables governing the sampling rate of the sensor
uint8_t currentTime = 1; //variable used to hold "minute" reading of RTC
uint8_t lastTime = 1; //variable used to hold the previous "minute" reading of RTC
//See code later for more description of the above variables
int delayTime = 4; //time in minutes between between readings

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
  if (!ina219.begin()) { //these lines initialize the INA219 chip
    Serial.println("Failed to find INA219 chip...");
  }
  //To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  //Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  ina219.setCalibration_16V_400mA();

  //The below section powers on and sets up the SIM7000 chip so that the
  //package can connect to the cellular network
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH); // Default state
  //This function is declared below and powers on the Botletics shield
  BotleticsSetup();
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
    //If not, then the SIM7000 shield will reboot. All details found in the Void MQTT connect section below
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
        dataFile.println("Ultrasonic 1 Height Reading (ft), Ultrasonic 2 Height Reading (ft), Battery Voltage (V), Internal Temperature (F), Timestamp");
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

    //This creates string variables from numbers for transmission to Adafruit IO
    dtostrf(height1, 1, 2, dist1Buff);
    dtostrf(height2, 1, 2, dist2Buff);
    dtostrf(busVoltage, 1, 2, battBuff);
    dtostrf(tempF, 1, 2, tempBuff);

    //This section gauges the signal strength of the cellular connection
    // Read the raw RSSI value
    uint8_t n = modem.getRSSI();
    Serial.print(n); Serial.println(F(" RSSI"));
    dtostrf(n, 1, 2, signalBuff); //This saves the signal strength in raw RSSI to a string for transmission

    //This section connects the device to Adafruit IO and publishes the variables
    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected). See the MQTT_connect
    // function definition further below.

    // Now publish all the data to different feeds!
    // The MQTT_publish_checkSuccess handles repetitive stuff.
    MQTT_publish_checkSuccess(sonar1, dist1Buff);
    MQTT_publish_checkSuccess(sonar2, dist2Buff);
    MQTT_publish_checkSuccess(batteryVoltage, battBuff);
    MQTT_publish_checkSuccess(internalTemperature, tempBuff);
    MQTT_publish_checkSuccess(signalStrength, signalBuff);
  // Delay until next post
    lastTime = currentTime; //This prevents the "if" loop from running again and again
    //in the same minute

    //Serial.println((char *)initialHeight.lastread); //uncomment to debug
    Serial.println("Going to sleep!");
    //pingCount = 0; //reset the ping count
    firstRun = false;
  } 
  //Between sampling times, the Arduino is sent into light sleep which helps to save on power
  //It wakes up every 4 seconds to check the time and goes back to sleep unless the sampling rate
  //conditions are met in the "if" loop
  else {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
     delay(5);
  }
}  

//Our custom code ends here. Everything below are premade function codes

//************************** Function Codes - Do not alter these! **************************
void BotleticsSetup() {
    //The following lines are for powering on and starting up 
  //the SIM7000 Botletics shield
  modem.powerOn(BOTLETICS_PWRKEY); // Power on the module
  moduleSetup(); // Establishes first-time serial comm and prints IMEI
  // Set modem to full functionality
  modem.setFunctionality(1); // AT+CFUN=1
  modem.setNetworkSettings(F("hologram")); // For Hologram SIM card
    #if !defined(SIMCOM_3G) && !defined(SIMCOM_7500) && !defined(SIMCOM_7600)
      // Disable data just to make sure it was actually off so that we can turn it on
      if (!modem.enableGPRS(false)) //Serial.println(F("Failed to disable data!"));
      // Turn on data
      while (!modem.enableGPRS(true)) {
        //Serial.println(F("Failed to enable data, retrying..."));
        delay(2000);
      }
      Serial.println(F("Enabled data!"));
    #endif
      
    //The following section connects the SIM card to the cellular network and enables data
    //so we can transmit to Adafruit IO
    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    while (!netStatus()) {
      Serial.println(F("Failed to connect to cell network, retrying..."));
      delay(2000);
    }
    Serial.println(F("Connected to cell network!"));
    // Open wireless connection if not already activated
    if (!modem.wirelessConnStatus()) {
      while (!modem.openWirelessConnection(true)) {
        Serial.println(F("Failed to enable connection, retrying..."));
        delay(2000);    
      }
      Serial.println(F("Enabled data!"));
    }
    else {
      Serial.println(F("Data already enabled!"));
    }
    Serial.println(F("---------------------"));
}
//This function is what activates the SIM7000 card in the Botletics shield
void moduleSetup() {
  // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
  // Press Arduino reset button if the module is still turning on and the board doesn't find it.
  // When the module is on it should communicate right after pressing reset

  // Software serial:
  modemSS.begin(115200); // Default SIM7000 shield baud rate

  Serial.println(F("Configuring to 9600 baud"));
  modemSS.println("AT+IPR=9600"); // Set baud rate
  delay(100); // Short pause to let the command run
  modemSS.begin(9600);
  if (! modem.begin(modemSS)) {
    Serial.println(F("Couldn't find modem"));
    modem.powerDown();
    return;    
  }

  type = modem.type();
  Serial.println(F("Modem is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case SIM800L:
      Serial.println(F("SIM800L")); break;
    case SIM800H:
      Serial.println(F("SIM800H")); break;
    case SIM808_V1:
      Serial.println(F("SIM808 (v1)")); break;
    case SIM808_V2:
      Serial.println(F("SIM808 (v2)")); break;
    case SIM5320A:
      Serial.println(F("SIM5320A (American)")); break;
    case SIM5320E:
      Serial.println(F("SIM5320E (European)")); break;
    case SIM7000:
      Serial.println(F("SIM7000")); break;
    case SIM7070:
      Serial.println(F("SIM7070")); break;
    case SIM7500:
      Serial.println(F("SIM7500")); break;
    case SIM7600:
      Serial.println(F("SIM7600")); break;
    default:
      Serial.println(F("???")); break;
  }
  
  // Print module IMEI number.
  uint8_t imeiLen = modem.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
}

//This function determines if the SIM card is connected to a cellular network
bool netStatus() {
  int n = modem.getNetworkStatus();
  
  Serial.print(F("Network status ")); Serial.print(n); Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) Serial.println(F("Registered (home)"));
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));

  if (!(n == 1 || n == 5)) return false;
  else return true;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.println("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
      modem.powerDown();
      delay(10000);
      BotleticsSetup();
  }
  Serial.println("MQTT Connected!");
}

//This function publishes data to the Adafruit IO server
void MQTT_publish_checkSuccess(Adafruit_MQTT_Publish &feed, const char *feedContent) {
  Serial.println(F("Sending data..."));
   if (!feed.publish(feedContent)) {
    Serial.println(F("Failed"));
  }
  else {
    Serial.println(F("OK!"));
    txfailures = 0;
  }
}