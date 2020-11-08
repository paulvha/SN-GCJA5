/*
  Version 1.0 / November 2020 / paulvha
  - initial version

  Sketch to read SPS30, SN-GCJA5 and SDS-011 at the same time
  Output can easily be included in spreadsheet

  This example shows how to output the "mass densities" of PM1, PM2.5, and PM10 readings as well as the
  "particle counts" for particles 0.5 to 10 microns in size.

  What is PM and why does it matter?
  https://www.epa.gov/pm-pollution/particulate-matter-pm-basics

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/17123

  Hardware Connections:

    SPS30    SN-GCJA5      SDS011   Mega2560
    VCC --------  VCC ------ VCC --- 5V
    GND --------- GND ------ GND --  GND
    TX  ---------------------------  RX1
    RX  --------------------------   TX1
    Select                                  (not connected, serial comms selected)
                  SDA  -----------   SDA
                  SCL  -----------   SCL
                  Serial                    (not connected)
                            Tx ----- RX3
                            RX ----- TX3


  Open the serial monitor at 115200 baud

   ================================ Disclaimer ======================================
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  ===================================================================================
  NO support, delivered as is, have fun, good luck !!
*/

#include <Wire.h>
#include "SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Particle_SN-GCJA5
#include "sps30.h"  //download from https://github.com/paulvha/sps30
#include <NovaSDS011.h>

/*
Change to NovaSDS011 library

// in cpp file (removed softserial as it did not work on MEGA2560) and now use Serial3
void NovaSDS011::begin(Stream *serialPort, uint8_t pin_rx, uint8_t pin_tx, uint16_t wait_write_read)
{
  _waitWriteRead = wait_write_read;
  // SoftwareSerial *softSerial = new SoftwareSerial(pin_rx, pin_tx);

  // Initialize soft serial bus
  // softSerial->begin(9600);
  // _sdsSerial = softSerial;
  _sdsSerial = serialPort;
  clearSerial();
}

in .h file change the call
void NovaSDS011::begin(Stream *serialPort, uint8_t pin_rx, uint8_t pin_tx, uint16_t wait_write_read)
*/

/////////////////////////////////////////////////////////////
/*define communication channel to use for SPS30
 valid options:
 *   I2C_COMMS              use I2C communication
 *   SOFTWARE_SERIAL        Arduino variants (NOTE)
 *   SERIALPORT             ONLY IF there is NO monitor attached
 *   SERIALPORT1            Arduino MEGA2560, Due. Sparkfun ESP32 Thing : MUST define new pins as defaults are used for flash memory)
 *   SERIALPORT2            Arduino MEGA2560, Due and ESP32
 *   SERIALPORT3            Arduino MEGA2560 and Due only for now

 * NOTE: Softserial has been left in as an option, but as the SPS30 is only
 * working on 115K the connection will probably NOT work on any device. */
/////////////////////////////////////////////////////////////
#define SP30_COMMS SERIALPORT1

/////////////////////////////////////////////////////////////
/* define RX and TX pin for softserial and Serial1 on ESP32
 * can be set to zero if not applicable / needed           */
/////////////////////////////////////////////////////////////
#define TX_PIN 26
#define RX_PIN 25

/////////////////////////////////////////////////////////////
/* define SPS30 driver debug
 * 0 : no messages
 * 1 : request sending and receiving
 * 2 : request sending and receiving + show protocol errors */
 //////////////////////////////////////////////////////////////
#define DEBUG 0

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

// create constructors
SPS30 sps30;
SFE_PARTICLE_SENSOR myAirSensor;
NovaSDS011 sds011;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Compare Panasonic SN-GCJA5, SPS30 and SDS011 Example"));

  Wire.begin();

  //////////////////////// START SN_GCJA5 /////////////////////////
  if (myAirSensor.begin() == false)
  {
    Serial.println("The particle sensor did not respond. Please check wiring. Freezing...");
    while (1);
  }

  ////////////////////// Start SPS30 ///////////////////////////////
  // set driver debug level
  sps30.EnableDebugging(DEBUG);

  // set pins to use for softserial and Serial1 on ESP32
  if (TX_PIN != 0 && RX_PIN != 0) sps30.SetSerialPin(RX_PIN,TX_PIN);

  // Begin communication channel;
  if (! sps30.begin(SP30_COMMS))
    Errorloop((char *) "could not initialize communication channel.", 0);

  // check for SPS30 connection
  if (! sps30.probe()) Errorloop((char *) "could not probe / connect with SPS30.", 0);
  else  Serial.println(F("Detected SPS30."));

  // reset SPS30 connection
  if (! sps30.reset()) Errorloop((char *) "could not reset.", 0);

  // read device info
  GetDeviceInfo();

  /////////////////// start SDS011 ////////////////////////////////
  Serial3.begin(9600);
  sds011.begin(&Serial3,2, 3);      // needs change in library see top of sketch the values 2 and 3 are ignored and only Serial3 is used!!

  if (sds011.setWorkingMode(WorkingMode::work))
  {
    Serial.println("SDS011 working mode \"Work\"");
  }
  else
  {
    Serial.println("FAIL: Unable to set working mode \"Work\"");
  }


  SDS011Version version = sds011.getVersionDate();

  if (version.valid)
  {
    Serial.println("SDS011 Firmware Vesion:\nYear: " + String(version.year) + "\nMonth: " +
                   String(version.month) + "\nDay: " + String(version.day));
  }
  else
  {
    Serial.println("FAIL: Unable to obtain Software Version");
  }

  if (sds011.setDutyCycle(0))
  {
    Serial.println("SDS011 Duty Cycle set to constant report");
  }
  else
  {
    Serial.println("FAIL: Unable to set Duty Cycle");
  }

  //////////////////// start measurement ////////////////////////////
  if (sps30.start()) Serial.println(F("Measurement started"));
  else Errorloop((char *) "Could NOT start measurement", 0);

  serialTrigger((char *) "Hit <enter> to continue reading");

  if (SP30_COMMS == I2C_COMMS) {
    if (sps30.I2C_expect() == 4)
      Serial.println(F(" !!! Due to I2C buffersize only the SPS30 MASS concentration is available !!! \n"));
  }
  Serial.println("Sensor started");
}

/**
 * @brief : read and display device info
 */
void GetDeviceInfo()
{
  char buf[32];
  uint8_t ret;
  SPS30_version v;

  //try to read serial number
  ret = sps30.GetSerialNumber(buf, 32);
  if (ret == ERR_OK) {
    Serial.print(F("Serial number : "));
    if(strlen(buf) > 0)  Serial.println(buf);
    else Serial.println(F("not available"));
  }
  else
    ErrtoMess((char *) "could not get serial number", ret);

  // try to get product name
  ret = sps30.GetProductName(buf, 32);
  if (ret == ERR_OK)  {
    Serial.print(F("Product name  : "));

    if(strlen(buf) > 0)  Serial.println(buf);
    else Serial.println(F("not available"));
  }
  else
    ErrtoMess((char *) "could not get product name.", ret);


  // try to get version info
  ret = sps30.GetVersion(&v);
  if (ret != ERR_OK) {
    Serial.println(F("Can not read version info"));
    return;
  }

  Serial.print(F("Firmware level: "));  Serial.print(v.major);
  Serial.print("."); Serial.println(v.minor);

  if (SP30_COMMS != I2C_COMMS) {
    Serial.print(F("Hardware level: ")); Serial.println(v.HW_version);

    Serial.print(F("SHDLC protocol: ")); Serial.print(v.SHDLC_major);
    Serial.print("."); Serial.println(v.SHDLC_minor);
  }

  Serial.print(F("Library level : "));  Serial.print(v.DRV_major);
  Serial.print(".");  Serial.println(v.DRV_minor);
}

void loop()
{

  read_all();

  delay(10000);   // 10 seconds

  return;
}

bool read_all()
{
  static bool header = true;
  float val, val_tot;
  uint16_t val_t, val_cnt;
  uint8_t ret, error_cnt = 0;
  struct sps_values v;

  // loop to get data
  do {

    ret = sps30.GetValues(&v);

    // data might not have been ready or value is 0 (can happen at start)
    if (ret == ERR_DATALENGTH || v.MassPM1 == 0 ) {

        if (error_cnt++ > 3) {
          ErrtoMess((char *) "Error during reading values: ",ret);
          return(false);
        }
        delay(1000);
    }

    // if other error
    else if(ret != ERR_OK) {
      ErrtoMess((char *) "Error during reading values: ",ret);
      return(false);
    }

  } while (ret != ERR_OK);

  // only print header first time
  if (header) {
    Serial.print(F("\tPM1.0\tPM2.5\tPM10\tPM0.5\tPM1.0\tPM2.5\tPM10\t\tPM1.0\tPM2.5\tPM10\tPM0.5\tPM1.0\tPM2.5\tPM10\t\tPM2.5\tPM10"));
    Serial.println();
    header = false;

    // often seen the first reading to be "out of bounds". so we skip it
    return(true);
  }

  Serial.print("GCJA5\t");
  Serial.print(myAirSensor.getPM1_0(),2);
  Serial.print("\t");
  Serial.print(myAirSensor.getPM2_5(),2);
  Serial.print("\t");
  Serial.print(myAirSensor.getPM10(),2);
  Serial.print("\t");
  val = myAirSensor.getPC0_5();     // 0.3 - 0.5um
  Serial.print(val,2);
  Serial.print("\t");
  val += myAirSensor.getPC1_0();    // 0.5 - 1 um
  Serial.print(val,2);
  Serial.print("\t");
  val += myAirSensor.getPC2_5();    // 1 - 2.5 um
  Serial.print(val,2);
  Serial.print("\t");

  val += myAirSensor.getPC5_0();    // 2.5 - 5 um
  val += myAirSensor.getPC7_5();    // 5 - 7.5 um
  val += myAirSensor.getPC10();     // 7.5 - 10 um)
  Serial.print(val,2);

  // sps30
  Serial.print("\tSPS30\t");

  Serial.print(v.MassPM1, 2);
  Serial.print("\t");
  Serial.print(v.MassPM2, 2);
  Serial.print("\t");
  Serial.print(v.MassPM10, 2);
  Serial.print("\t");
  Serial.print(v.NumPM0, 2);      // 0.3 - -0.5um
  Serial.print("\t");
  Serial.print(v.NumPM1, 2);      // 0.3 - 1 um
  Serial.print("\t");
  Serial.print(v.NumPM2, 2);      // 0.3 - 2.5um
  Serial.print("\t");
  Serial.print(v.NumPM10, 2);     // 0.3 - 10 um


  float p25, p10;
  if (sds011.queryData(p25, p10) == QuerryError::no_error)
  {
    Serial.print("\tSDS011\t");
    Serial.print(p25, 2);
    Serial.print("\t");
    Serial.print(p10, 2);
  }

  Serial.println();
  return(true);
}

/**
 *  @brief : continued loop after fatal error
 *  @param mess : message to display
 *  @param r : error code
 *  if r is zero, it will only display the message
 */
void Errorloop(char *mess, uint8_t r)
{
  if (r) ErrtoMess(mess, r);
  else Serial.println(mess);
  Serial.println(F("Program on hold"));
  for(;;) delay(100000);
}

/**
 *  @brief : display error message
 *  @param mess : message to display
 *  @param r : error code
 */
void ErrtoMess(char *mess, uint8_t r)
{
  char buf[80];

  Serial.print(mess);

  sps30.GetErrDescription(r, buf, 80);
  Serial.println(buf);
}

/**
 * serialTrigger prints repeated message, then waits for enter
 * to come in from the serial port.
 */
void serialTrigger(char * mess)
{
  Serial.println();

  while (!Serial.available()) {
    Serial.println(mess);
    delay(2000);
  }

  while (Serial.available())
    Serial.read();
}
