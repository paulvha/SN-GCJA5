/*
  Version 1.0 / November 2020 / paulvha
  - initial version

  This sketch will read the SN-GCJA5 with I2C and / or Serial
  Both can be used at the same time

  The output is either in a single row (seperated with tabs) or double
  underneath each other.

  Hardware connection used
  SN-GCJA5       Mega2560
  VCC ------------- 5V
  GND ------------  GND
  SCL  -----------  SCL
  SDA  -----------  SDA
  Serial ---------  RX2  / pin 17   Serial2

  Open the serial monitor at 115200 baud

  ================================ Disclaimer ======================================
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  ===================================================================================
  NO support, delivered as is, have fun, good luck !!
*/

//////////////////////////////////////////////////////////
//   Select communication to use                        //
//////////////////////////////////////////////////////////
#define SERIALCOMMS  true
#define I2COMMS true

//////////////////////////////////////////////////////////
//   Select layout to use
//   true :  results next to each other to easily copy in spreadsheet
//   false : results underneath each other
//////////////////////////////////////////////////////////
bool SpreadSheetLayout = true;

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

// checks will happen at pre-processor time to
#if SERIALCOMMS != true && I2COMMS != true
#error "Select communication to use"
#endif

#if SERIALCOMMS == true
// Serial settings
#define STX 0x02
#define ETX 0x03
#define SenCom Serial2      // we use Serial2 on a Mega2560

uint8_t data[40];
uint8_t offset;

// offset in data-buffer
#define SER_SNGCJA5_PM1_0 1
#define SER_SNGCJA5_PM2_5 5
#define SER_SNGCJA5_PM10  9
#define SER_SNGCJA5_PCOUNT_0_5 13
#define SER_SNGCJA5_PCOUNT_1_0 15
#define SER_SNGCJA5_PCOUNT_2_5 17
#define SER_SNGCJA5_PCOUNT_5_0 21
#define SER_SNGCJA5_PCOUNT_7_5 23
#define SER_SNGCJA5_PCOUNT_10  25
#define SER_SNGCJA5_STATE      29

#endif

#if I2COMMS == true
// wire settings
#include "SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Particle_SN-GCJA5
SFE_PARTICLE_SENSOR myAirSensor;
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Panasonic SN-GCJA5 Serial / I2c Example"));

#if I2COMMS == true
  Wire.begin();

  if (myAirSensor.begin() == false)
  {
    Serial.println("The particle sensor did not respond. Please check wiring. Freezing...");
    while (1);
  }

  Serial.println(F("The particle sensor SN-GCJA5 detected"));
#endif

#if SERIALCOMMS == true
  SenCom.begin(9600, SERIAL_8E1);
  offset = 0;
#endif

  Serial.println("Sensor started");
}

void loop()
{
  read_all();
  delay(1000);
}

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

#if SERIALCOMMS == true
/* used for serial translation
 * the PM1, PM2.5 and PM10 values are NOT uint32, they are actually 16 bit values
 * where I2C communication will show 1.23456.. the serial will only show 1
 * only the number before the decision point... BUT still takes 4 bytes !!
 */
uint32_t bufto32(uint8_t p)
{
  return(((uint32_t)data[p+3]<< 24) | ((uint32_t)data[p+2] << 16) | ((uint32_t)data[p+1] << 8) | ((uint32_t)data[p]<< 0));
}

/* for serial translation
 * the particle count is 16 bits as on I2C transmission
 */
uint16_t bufto16(uint8_t p)
{
  return ((uint16_t)data[p+1] << 8 | data[p]);
}
#endif

void read_all()
{
  bool SerComplete = false;

#if SERIALCOMMS == true
  offset = 0;

  // clear anything pending in the serial buffer
  while (SenCom.available()) SenCom.read();

  // wait for new data to arrive (each second)
  delay(1100);

  // read data from serial port
  if(SenCom.available()){

    while (SenCom.available()){

      data[offset] = SenCom.read();

      // check for start
      if (offset == 0) {
        if (data[offset] != STX) continue;
      }

      // check for end of message
      if (offset == 31) {

        uint8_t FCC = 0;
        for ( int i = 1 ; i < 30 ; i ++ )
            FCC = FCC ^ data[i];

        if (data[offset] != ETX || FCC != data[30] ) {
          Serial.println("Packet error");
          offset = 0;
          continue;
        }

        SerComplete = true;    // we have valid data
        break;
      }

      // next position
      offset++;
    }
  }

/*
  // for debug only
  for (int i =0; i < 32;i++){
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println();
*/
#endif // SN_SERIALCOMMS

  // print result on one line, easy for spreadsheet == true
  if (SpreadSheetLayout) display_spreadSheet(SerComplete);

  // print result underneath each other
  else display_line(SerComplete);
}

//////////////////////////////////////////////////////////////////////
/////////////////////// DISPLAY ROUTINES /////////////////////////////
//////////////////////////////////////////////////////////////////////

// display underneath each other
void display_line(bool SerComplete)
{
  static bool header = true;
  float val, val_t, val_tot, val_cnt;

  // only print header first time
  if (header) {
    Serial.println(F("\t----------------------------Mass -------------    -------------------------------- Number ---------------------------------  -- status --"));
    Serial.println(F("\t                     Concentration [μg/m3]                                 Concentration [#/cm3]                    [μm] "));
    Serial.print(F("\t PM1.0             PM2.5           PM10               PM0.5           PM1.0           PM2.5           PM10           Average "));
    Serial.println();
    header = false;

    // often seen the first reading to be "out of bounds". so we skip it
    return(true);
  }

 #if I2COMMS == true
  Serial.print("I2c\t");
  print_aligned((double) myAirSensor.getPM1_0(), 8, 5);
  print_aligned((double) myAirSensor.getPM2_5(), 8, 5);
  print_aligned((double) myAirSensor.getPM10(), 8, 5);

  val = myAirSensor.getPC0_5();
  val_tot = val * 0.5;
  val_cnt = val;
  print_aligned((double) val, 9, 2);

  val_t = myAirSensor.getPC1_0();
  val += val_t;
  val_tot += val * 1;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  val_t = myAirSensor.getPC2_5();
  val += val_t;
  val_tot += val * 2.5;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  val_t = myAirSensor.getPC5_0();
  val += val_t;
  val_tot += val * 5;
  val_cnt += val_t;

  val_t = myAirSensor.getPC7_5();
  val += val_t;
  val_tot += val * 7.5;
  val_cnt += val_t;

  val_t = myAirSensor.getPC10();
  val += val_t;
  val_tot += val * 10;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  val = val_tot / val_cnt;
  print_aligned((double) val, 9, 2);

  Serial.print(" ");
  Serial.println(myAirSensor.getState(), HEX);
#endif

#if SERIALCOMMS == true
  Serial.print("Ser\t");
  print_aligned((double) bufto32(SER_SNGCJA5_PM1_0), 8, 5);
  print_aligned((double) bufto32(SER_SNGCJA5_PM2_5), 8, 5);
  print_aligned((double) bufto32(SER_SNGCJA5_PM10), 8, 5);

  val = bufto16(SER_SNGCJA5_PCOUNT_0_5);
  val_tot = val * 0.5;
  val_cnt = val;
  print_aligned((double) val, 9, 2);

  val_t = bufto16(SER_SNGCJA5_PCOUNT_1_0);
  val += val_t;
  val_tot += val * 1;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  val_t =bufto16(SER_SNGCJA5_PCOUNT_2_5);
  val += val_t;
  val_tot += val * 2.5;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  val_t = bufto16(SER_SNGCJA5_PCOUNT_5_0);
  val += val_t;
  val_tot += val * 5;
  val_cnt += val_t;

  val_t = bufto16(SER_SNGCJA5_PCOUNT_10);
  val += val_t;
  val_tot += val * 7.5;
  val_cnt += val_t;

  val_t = bufto16(SER_SNGCJA5_PCOUNT_10);
  val += val_t;
  val_tot += val * 10;
  val_cnt += val_t;
  print_aligned((double) val, 9, 2);

  val = val_tot / val_cnt;
  print_aligned((double) val, 9, 2);
  Serial.print(" ");
  Serial.print(data[SER_SNGCJA5_STATE], HEX);

  if (! SerComplete) Serial.print("\told");

  Serial.println();
#endif
}

// display result easy for spreadsheet copy / paste
void display_spreadSheet(bool SerComplete)
{
  static bool header = true;
  float val;
  uint16_t ival;

  // only print header first time
  if (header) {

    #if SERIALCOMMS == true
    Serial.print(F("\t------ Mass -------- \t------- Particle Count ------\t"));
    #endif

    #if I2COMMS == true
    Serial.print(F("\t------ Mass -------- \t------- Particle Count ------\t"));
    #endif

    Serial.println();

    #if SERIALCOMMS == true
    Serial.print(F("\tPM1.0\tPM2.5\tPM10\tPM0.5\tPM1.0\tPM2.5\tPM10\t"));
    #endif

    #if I2COMMS == true
    Serial.print(F("\tPM1.0\tPM2.5\tPM10\tPM0.5\tPM1.0\tPM2.5\tPM10\t"));
    #endif

    Serial.println();
    header = false;

    // often seen the first reading to be "out of bounds". so we skip it
    return(true);
  }


#if I2COMMS == true

  Serial.print("I2c\t");
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
  Serial.print("\t");
#endif

#if SERIALCOMMS == true
  Serial.print("Ser\t");
  Serial.print(bufto16(SER_SNGCJA5_PM1_0));
  Serial.print("\t");
  Serial.print(bufto16(SER_SNGCJA5_PM2_5));
  Serial.print("\t");
  Serial.print(bufto16(SER_SNGCJA5_PM10));
  Serial.print("\t");

  ival = bufto16(SER_SNGCJA5_PCOUNT_0_5);    // 0.3 - 0.5um
  Serial.print(ival);
  Serial.print("\t");

  ival += bufto16(SER_SNGCJA5_PCOUNT_1_0);    // 0.5 - 1 um
  Serial.print(ival);
  Serial.print("\t");

  ival += bufto16(SER_SNGCJA5_PCOUNT_2_5);    // 1 - 2.5 um
  Serial.print(ival);
  Serial.print("\t");

  ival += bufto16(SER_SNGCJA5_PCOUNT_5_0);    // 2.5 - 5 um
  ival += bufto16(SER_SNGCJA5_PCOUNT_7_5);    // 5 - 7.5 um
  ival += bufto16(SER_SNGCJA5_PCOUNT_10);    // 7.5 - 10 um

  Serial.print(ival);

  if (! SerComplete) Serial.print("\told");

#endif

  Serial.println();
}
