/*
  Version 1.0 / November 2020 / paulvha
  - initial version

  This example will compare the SPS30 and SN-GCJA5 output side by side.

  The output is either in a single row (seperated with tabs) or double
  underneath each other nicely formatted.

  This example shows how to output the "mass densities" of PM1, PM2.5, and PM10 readings as well as the
  "particle counts" for particles 0.5 to 10 microns in size.

  What is PM and why does it matter?
  https://www.epa.gov/pm-pollution/particulate-matter-pm-basics

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/17123

  Hardware Connections:
    SPS30    SN-GCJA5      Mega2560
    VCC --------  VCC ------- 5V
    GND --------- GND ------  GND
    TX  --------------------  RX1
    RX  --------------------  TX1
    Select                                (not connected, serial comms selected)
                  SCL  -----  SCL
                  SDA  -----  SDA
                  Serial                  (not connected)

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
#include "sps30.h" //  download from https://github.com/paulvha/sps30

//////////////////////////////////////////////////////////
//   Select layout to use
//   true :  results next to each other to easily copy in spreadsheet
//   false : results underneath each other
//////////////////////////////////////////////////////////
bool SpreadSheetLayout = true;

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

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Compare Panasonic SN-GCJA5 and SPS30 Example"));

  Wire.begin();

  if (myAirSensor.begin() == false)
  {
    Serial.println(F("The particle sensor did not respond. Please check wiring. Freezing..."));
    while (1);
  }

  Serial.println(F("The particle sensor SN-GCJA5 detected"));

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

  // start measurement
  if (sps30.start()) Serial.println(F("Measurement started"));
  else Errorloop((char *) "Could NOT start measurement", 0);

  serialTrigger((char *) "Hit <enter> to continue reading");

  if (SP30_COMMS == I2C_COMMS) {
    if (sps30.I2C_expect() == 4)
      Serial.println(F(" !!! Due to I2C buffersize only the SPS30 MASS concentration is available !!! \n"));
  }
  Serial.println("Sensor started");
}

void loop()
{
  read_all();
  delay(2000);
}

bool read_all()
{
  uint8_t ret, error_cnt = 0;
  struct sps_values v;

  // loop to get data from SPS30
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

  // print result on one line, easy for spreadsheet == true
  if (SpreadSheetLayout) display_spreadSheet(&v);

  // print result underneath each other
  else display_line(&v);

  return(true);
}

/**
 * @brief : read and display device info SPS30
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

//////////////////////////////////////////////////////////////////////
/////////////////////// DISPLAY ROUTINES /////////////////////////////
//////////////////////////////////////////////////////////////////////

/**
 * @brief will print nice aligned columns
 *
 * @param val : value to print
 * @param width : total width of value including decimal point
 * @param prec : precision after the decimal point
 */
void print_aligned(double val, signed char width, unsigned char prec)
{
  char out[15];
  dtostrf(val, width, prec, out);
  Serial.print(out);
  Serial.print(F("\t  "));
}

// display nice aligned underneath each other
void display_line(struct sps_values *v)
{
  static bool header = true;
  float val, val_tot;
  uint16_t val_t, val_cnt;

  // only print header first time
  if (header) {
    Serial.println(F("\t----------------------------Mass -------------    -------------------------------- Number ---------------------------------     -------Partsize -------- "));
    Serial.println(F("\t                     Concentration [μg/m3]                                 Concentration [#/cm3]                                           [μm] "));
    Serial.print(F("\t PM1.0             PM2.5           PM10             PM0.5           PM1.0           PM2.5           PM10          Typical         Average         "));
    Serial.println();
    header = false;

    // often seen the first reading to be "out of bounds". so we skip it
    return(true);
  }

  Serial.print("GCJA5\t");

  print_aligned((double) myAirSensor.getPM1_0(), 8, 5);
  print_aligned((double) myAirSensor.getPM2_5(), 8, 5);
  print_aligned((double) myAirSensor.getPM10(), 8, 5);

  val = myAirSensor.getPC0_5();
  val_tot = (float) val * (float) 0.5;
  val_cnt = val;
  print_aligned((double) val, 9, 2);

  val_t = myAirSensor.getPC1_0();
  val += val_t;
  val_tot += (float) val_t * (float) 1;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  val_t = myAirSensor.getPC2_5();
  val += val_t;
  val_tot += (float) val_t * (float) 2.5;
  val_cnt += (float) val_t;
  print_aligned((double) val, 9, 2);

  val_t = myAirSensor.getPC5_0();
  val += val_t;
  val_tot += (float) val_t *(float)  5;
  val_cnt += val_t;

  val_t = myAirSensor.getPC7_5();
  val += val_t;
  val_tot += (float) val_t *(float)  7.5;
  val_cnt += val_t;

  val_t = myAirSensor.getPC10();
  val += val_t;
  val_tot += (float) val_t * (float) 10;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  Serial.print("\t\t");
  val = val_tot / val_cnt;
  print_aligned((double) val, 9, 2);

  // sps30
  Serial.print("\nSPS30\t");
  print_aligned((double) v->MassPM1, 8, 5);
  print_aligned((double) v->MassPM2, 8, 5);
  print_aligned((double) v->MassPM10, 8, 5);
  print_aligned((double) v->NumPM0, 9, 2);
  print_aligned((double) v->NumPM1, 9, 2);
  print_aligned((double) v->NumPM2, 9, 2);
  print_aligned((double) v->NumPM10, 9, 2);
  print_aligned((double) v->PartSize, 7, 5);

  double tmp = calc_avg(v);
  print_aligned(tmp, 7, 5);

  Serial.println("\n");
}

// display result easy for spreadsheet copy / paste
void display_spreadSheet(struct sps_values *v)
{
  static bool header = true;
  float val;

  // only print header first time
  if (header) {
    Serial.println(F("\t------ Mass -------- \t------- Particle Count ------\t\t------ Mass -------- \t------- Particle Count ------\t"));
    Serial.print(F("\tPM1.0\tPM2.5\tPM10\tPM0.5\tPM1.0\tPM2.5\tPM10\t\tPM1.0\tPM2.5\tPM10\tPM0.5\tPM1.0\tPM2.5\tPM10\n"));
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
  val += myAirSensor.getPC10();     // 7.5 - 10 um
  Serial.print(val,2);

  // sps30
  Serial.print("\tSPS30\t");

  Serial.print(v->MassPM1, 2);
  Serial.print("\t");
  Serial.print(v->MassPM2, 2);
  Serial.print("\t");
  Serial.print(v->MassPM10, 2);
  Serial.print("\t");
  Serial.print(v->NumPM0, 2);      // 0.3 - -0.5um
  Serial.print("\t");
  Serial.print(v->NumPM1, 2);      // 0.3 - 1 um
  Serial.print("\t");
  Serial.print(v->NumPM2, 2);      // 0.3 - 2.5um
  Serial.print("\t");
  Serial.print(v->NumPM10, 2);     // 0.3 - 10 um
  Serial.println();
}

/**
 * According to the datasheet: PMx defines particles with a size smaller than “x” micrometers (e.g., PM2.5 = particles smaller than 2.5 μm).
 *
 *assume :  PM0.5   PM1    PM2.5  PM4    PM10       Typical size
 *          30.75 / 35.2 / 35.4 / 35.4 / 35.4 #/cm³ -> 0.54μm
 *
 * That means (taking the samples mentioned above):
 * 30.75 have a size up to 0.5 um               >> avg. size impact = 30.75 * 0.499
 * 35.2 - 30.75 have a size between 0.5 and 1   >> avg. size impact = (35.2 - 30.75) * 0.99
 * 35.4 - 35.2 have a size between 1 and 2.5um >> etc
 *
 * Add the avg. size impact values ( 20.325) and divide by total = PM10 = 35.4) gives a calculated avg size of 0.57.
 *
 * PM0.5  PM1   PM2.5   PM4   PM10  avg size
 * 30.75   35.2  35.4  35.4  35.4  0.54
 * 0.499   0.99  2.49  3.99  9.99
 * 15.345  4.40  0.498    0    0   20.247
 *
 *                   Calculated average : 0.5719
 *
 * It is not a 100% fit. Maybe they apply different multiplier for size impact, maybe have more information in the sensor than exposed,
 * maybe include a number of the previous measurements in the calculations to prevent the number jump up and down too much
 * between the snap-shots.

 * I had a sketch running for 175 samples, sample every 3 seconds
 * The average for the 175 typical size was : 0,575451860465117 um
 * The average for 175 calculated avg was : 0,583083779069767 um,
 * Thus a delta of 0,007631918604651 over 175 samples. The error margin of 1.33%. I can live with that.
 *
 * One suprising aspect is when float's were used to calculate often 0.57620 is the outcome... When checking the values and calculations
 * with a spreadsheet, there was a mismatch in the result . (error with float measurement ?)
 *
 * Hence the double values are applied.
 *
 */
double calc_avg(struct sps_values *v)
{
  double a,b;

  a = (double) v->NumPM0 * (double) 0.499;
  /*Serial.print(F("\n a: "));
  Serial.print(a);
  Serial.print(F("\t a: "));
*/
  b = (double) (v->NumPM1  - v->NumPM0);
  a += b * (double) 0.99;
  /*Serial.print(a);
  Serial.print(F("\t b "));
  Serial.print(b);
  Serial.print(F("\t a "));
*/
  b = (double) (v->NumPM2  - v->NumPM1);
  a += b * (double) 2.49;
  /*Serial.print(a);
  Serial.print(F("\t b "));
  Serial.print(b);
  Serial.print(F("\t a "));;
  */
  b = (double) (v->NumPM4  - v->NumPM2);
  a += b * (double) 3.99;
  /*Serial.print(a);
  Serial.print(F("\t b "));
  Serial.print(b);
  Serial.print(F("\t a "));
*/
  b = (double) (v->NumPM10  - v->NumPM4);
  a += b * (double) 9.99;
  /*Serial.print(a);
  Serial.print(F("\t b "));
  Serial.print(b);
  Serial.print(F("\t a "));

  Serial.print(a);
  Serial.print(F("\t b "));
  Serial.print(b);
  Serial.print(F("\n"));
  */
  return(a / (double) v->NumPM10);
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
