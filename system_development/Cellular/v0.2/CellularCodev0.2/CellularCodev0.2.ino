// Based on code from Timothy Woo (www.botletics.com)  /Github: https://github.com/botletics/SIM7000-LTE-Shield
//Also based on code from Adafruit - www.Adafruit.com
//Credit given to all creators of included libraries
//This project is open source
//Remote Cellular Water Height Sensor developed by Nicholas Liger, from previous work by Corrine Smith and Winford Janvrin
//in conjuction with ARTS - LAB at University of South and Carolina and South Carolina Department of Health and Environmental Control (SCDHEC)

/************************* All Included Libraries - Do not change this section! *********************************/

#include "BotleticsSIM7000.h" // Botletics SIM7000 library developed by Timothy Woo
#include "Adafruit_MQTT.h" //Adafruit library for connection via MQTT to the Adafruit IO server
#include "Adafruit_MQTT_FONA.h" //Adafruit library for using cellular data to connect to Adafruit IO server. 
//See Readme in Special Libraries folder for more information on formatting the above three libraries
#include <SPI.h>  //SPI communication library
#include <SD.h> //SD card library
#include <Adafruit_INA219.h> //Adafruit library for controlling INA219 voltage/current sensing chip
#include "NewPing.h" //New Ping library for use with the sonar sensors
#include <Wire.h> //Wire library for communicating with I2C and the RTC
#include <DS3231.h> //library for RTC module
#include <LowPower.h>

/************************* Botletics SIM7000 Set Up *********************************/
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


/************************* Adafruit.io Setup *********************************/
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
/****************************** Publishing Feeds for the Adafruit IO***************************************/
//All Publish paths are for sending data from the Arduino to the Adafruit Server
Adafruit_MQTT_Publish sonar1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sonar-sensor-1");
Adafruit_MQTT_Publish sonar2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sonar-sensor-2");
Adafruit_MQTT_Publish batteryVoltage = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/battery-voltage");

/****************************** Declaring Other Variables ***************************************/
//random variables for running Botletics scripts
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
char imei[16] = {0}; // Use this for device ID
uint8_t type;


// Char/String variables for holding the battery and height variables
//for transmitting to Adafruit IO
char battBuff[12],dist1Buff[12],dist2Buff[12];

//Variables for setting up the INA219 power monitoring device
Adafruit_INA219 ina219; //declares the INA219 variable, do not edit
float busVoltage = 0; //the voltage reading on the battery input from INA219

//Variables for setting up the two ultrasonic sensors
const int trigPin1 = 47; //trigger pin for sonar sensor 1
const int echoPin1 = 46; //echo pin for sonar sensor 1
const int trigPin2 = 45; //trigger pin for sonar sensor 2
const int echoPin2 = 44; //echo pin for sonar sensor 2
const int maxDist = 450; // max distance for sonar sensor, same for both
NewPing sonar_sensor_1(trigPin1, echoPin1, maxDist); //set up NewPing variable for sonar sensor 1
NewPing sonar_sensor_2(trigPin2, echoPin2, maxDist); //set up NewPing variable for sonar sensor 2
float distance1=0; //variable for distance measured by sonar sensor 1 used to publish to Adafruit IO via MQTT 
float distance2=0; //variable for distance measured by sonar sensor 2 used to publish to Adafruit IO via MQTT 
float origDist1=0; //Original distance for sensor 1 that is used to calibrate the height measurement of the water
float origDist2=0; //Same as above for sensor 2
float origHeight1 = 0; //Height of the water beneath sensor 1, measured at the time of installation
//to calibrate the sensor readings
float origHeight2 = 0; //Same for sensor 2
float height1 = 0; //Height variable for the current water height calculated from distance1
float height2 = 0; //Same for sensor 2

//Variables for setting up the SD card and RTC
char timeStamp[32]; //Char/string variable for holding time stamp
const int chipSelect = 53; //Chip Select for the SD card
char dayStampFileName[20]; //Char/string variable for holding the day stamp for naming
//file in SD card
RTClib myRTC; //RTC variable for using DS3231 library scripts
uint8_t currentTime = 1;
uint8_t lastTime = 1;

//*******************************Void Set up************************************
void setup() {
  Serial.begin(9600); //begin the Serial monitor
  Wire.begin(); //begin communication with I2C, needed for RTC module
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

  //These following lines start up and calibrate the INA219 chip
  if (! ina219.begin()) { //these lines initialize the INA219 chip
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();

  //These two lines ping the sonar sensors one time to calibrate them with an original distance
  origDist1 = sonar_sensor_1.ping_cm();
  origDist2 = sonar_sensor_2.ping_cm();
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH); // Default state
}

void loop() {
  DateTime now = myRTC.now();  
  currentTime = now.minute();
  if (currentTime %5 == 0 && currentTime != lastTime ){
    //The following lines are for powering on and starting up 
    //the SIM7000 Botletics shield
    modem.powerOn(BOTLETICS_PWRKEY); // Power on the module
    moduleSetup(); // Establishes first-time serial comm and prints IMEI
    // Set modem to full functionality
    modem.setFunctionality(1); // AT+CFUN=1
    modem.setNetworkSettings(F("hologram")); // For Hologram SIM card
    #if !defined(SIMCOM_3G) && !defined(SIMCOM_7500) && !defined(SIMCOM_7600)
      // Disable data just to make sure it was actually off so that we can turn it on
      if (!modem.enableGPRS(false)) Serial.println(F("Failed to disable data!"));
      // Turn on data
      while (!modem.enableGPRS(true)) {
        Serial.println(F("Failed to enable data, retrying..."));
        delay(2000); // Retry every 2s
      }
      Serial.println(F("Enabled data!"));
    #endif
    //The following section connects the SIM card to the cellular network and enables data
    //so we can transmit to Adafruit IO
    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    while (!netStatus()) {
      Serial.println(F("Failed to connect to cell network, retrying..."));
      delay(2000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));
    // Open wireless connection if not already activated
    if (!modem.wirelessConnStatus()) {
      while (!modem.openWirelessConnection(true)) {
        Serial.println(F("Failed to enable connection, retrying..."));
        delay(2000); // Retry every 2s
      }
      Serial.println(F("Enabled data!"));
    }
    else {
      Serial.println(F("Data already enabled!"));
    }
    Serial.println(F("---------------------"));

    //This section creates the timestamps from the RTC module
    sprintf(timeStamp, "%02d:%02d:%02d %02d/%02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());  
    snprintf(dayStampFileName, 20, "%02d%02d%02d.txt", now.day(), now.month(), now.year());  
    Serial.print(F("Date/Time: "));
    Serial.println(timeStamp);

    //This line gets the voltage of the battery from the INA219 chip
    busVoltage = ina219.getBusVoltage_V()+0.8; //get voltage from battery and add 0.8V to account for drop across diode

    //This section gets the current distance from the sonar sensors and calculates
    //the current height based on the the original distance of the sonar sensor and original height of the water
    // h = h0 + (d0-d)
    distance1 = sonar_sensor_1.ping_cm();
    distance2 = sonar_sensor_2.ping_cm();
    height1 = origHeight1 + (origDist1 - distance1)/(2.54*12);
    height2 = origHeight2 + (origDist2 - distance2)/(2.54*12);

    //This creates string variables from numbers for transmission to Adafruit IO
    dtostrf(height1, 1, 2, dist2Buff);
    dtostrf(height2, 1, 2, dist1Buff);
    dtostrf(busVoltage, 1, 2, battBuff);
    
    //This section connects the device to Adafruit IO and publishes the variables
    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected). See the MQTT_connect
    // function definition further below.
    MQTT_connect();
    // Now publish all the data to different feeds!
    // The MQTT_publish_checkSuccess handles repetitive stuff.
    MQTT_publish_checkSuccess(sonar1, dist1Buff);
    MQTT_publish_checkSuccess(sonar2, dist2Buff);
    MQTT_publish_checkSuccess(batteryVoltage, battBuff);
    

    // This section saves the data to the SD card
    Serial.println(dayStampFileName);
    //This line creates a new file named with the day time stamp (DDMMYYYY)
    //or opens the file if this name already exists
    //The SD cards have a tendency to become corrupted which can ruin a long term test
    //if the data is all saved to a single file, so this method creates a new CSV for every day
    //so that even if a file is corrupted, we will still have the data from every other day
    if (SD.exists(dayStampFileName) == 0) {
      File dataFile = SD.open(dayStampFileName, FILE_WRITE);
      if (dataFile){
        dataFile.println("Ultrasonic 1 Height Reading (ft), Ultrasonic 2 Height Reading (ft), Battery Voltage (V), Timestamp");
        dataFile.close();
      }
      else {
      Serial.println("error opening datalog.txt");
      }
    }
    File dataFile = SD.open(dayStampFileName, FILE_WRITE);
    if (dataFile) {
        dataFile.print(height1);
        dataFile.print(",");
        dataFile.print(height2);
        dataFile.print(",");
        dataFile.print(busVoltage);
        dataFile.print(",");
        dataFile.println(timeStamp);
        dataFile.close();
      }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    }

  // Delay until next post
    modem.powerDown();
    Serial.println("Powered Down!");
    lastTime = currentTime;
    delay(1000);
  } 
  else {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
  }
}

//************************** Function Code - Do not alter these! **************************
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
    while (1); // Don't proceed if it couldn't find the device
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
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}

//This function publishes data to the Adafruit IO server
void MQTT_publish_checkSuccess(Adafruit_MQTT_Publish &feed, const char *feedContent) {
  Serial.println(F("Sending data..."));
  if (! feed.publish(feedContent)) {
    Serial.println(F("Failed"));
    txfailures++;
  }
  else {
    Serial.println(F("OK!"));
    txfailures = 0;
  }
}
