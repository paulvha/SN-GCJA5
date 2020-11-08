/*
  Version 1.0 / November 2020 / paulvha
  - initial version
  
  This sketch will read the SN-GCJA5 with I2C and / or Serial
  Both can be used at the same time.
   
  The output is either in a single row (seperated with tabs) or double 
  underneath each other.  

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/17123
  
  The output is also sent to an LCD https://www.sparkfun.com/products/16396
  you can set alarm limits Low and HIGH alarm to change the color of the background
  you can decide to only display when alarm or when an optional button is pressed

  Hardware connection used
  SN-GCJA5       Mega2560   LCD
  VCC ------------- 5V ----  RAW 3V3 -9
  GND ------------  GND ---  GND
  SDA  -----------  SDA ---  DA
  SCL  -----------  SCL ---  CL
  Serial ---------  RX2                    / pin 17   Serial2

  Serial pin is close to edge of the sensor, VCC is most inside pin
  Open the serial monitor at 115200 baud

  LCD INFO **************************************************************************************
  https://www.sparkfun.com/products/16396
 
  The SparkFun SerLCD is an AVR-based, serial enabled LCD that provides a simple and cost
  effective solution for adding a 16x2 Black on RGB Liquid Crystal Display into your project.
  Both the SPS30 and LCD can be connected on the same WIRE device.
 
  The Qwiic adapter should be attached to the display as follows. If you have a model (board or LCD)
  without QWiic connect, or connect is indicated.
  Display  / Qwiic Cable Color        LCD -connection without Qwiic
  GND      / Black                    GND
  RAW      / Red                      3V3 -9 v
  SDA      / Blue                     I2c DA
  SCL      / Yellow                   I2C CL
 
  Note: If you connect directly to a 5V Arduino instead, you *MUST* use
  a level-shifter on SDA and SCL to convert the i2c voltage levels down to 3.3V for the display.
  !!!! Measured with a scope it turns out that the pull up is already to 3V3 !!!!
 
  If ONLYONBUTTON is set, connect a push-button switch between pin BUTTONINPUT and ground.
 
  
  ================================ Disclaimer ======================================
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  ===================================================================================
  NO support, delivered as is, have fun, good luck !!
*/

//////////////////////////////////////////////////////////
//   Select sensor communication to use                 //
//////////////////////////////////////////////////////////
#define SN_I2C_COMMS true
#define SN_SERIALCOMMS  true 
                       
//////////////////////////////////////////////////////////////////////////
//                SELECT LCD settings                                   //
//////////////////////////////////////////////////////////////////////////
// what are PM2.5 limits good , bad, ugly
//
// Every region in the world has it's own definition (sometimes multiple (like Canada)
// for Europe : https://www.airqualitynow.eu/download/CITEAIR-Comparing_Urban_Air_Quality_across_Borders.pdf
// for US : https://en.wikipedia.org/wiki/Air_quality_index#cite_note-aqi_basic-11
// for UK : https://en.wikipedia.org/wiki/Air_quality_index
// for India : https://en.wikipedia.org/wiki/Air_quality_index#cite_note-aqi_basic-11
// for canada : https://www.publichealthontario.ca/-/media/documents/air-quality-health-index.pdf?la=en
//  or          https://en.wikipedia.org/wiki/Air_Quality_Health_Index_(Canada)
//
// default PM2_LIMITLOW = 55 and PM2_LIMITHIGH = 110 is based on Europe
///////////////////////////////////////////////////////////////////////////
#define LCDCON Wire              // I2C channel to use for LCD

#define LCDBACKGROUNDCOLOR  1    // Normal background color: 1 = white, 2 = red, 3 = green 4 = blue 5 = off

#define LCDOUTPUTONLY false      // when set to true the results are only displayed on the LCD

float PM2_LIMITLOW = 55.0 ;      // Background LCD color will start LCDBACKGROUNDCOLOR and turn to blue if
                                 // PM2.5 is above this limit to return to LCDBACKGROUNDCOLOR background when below.
                                 // set to zero to disable

float PM2_LIMITHIGH = 110.0 ;    // Background LCD color will start LCDBACKGROUNDCOLOR and turn to red if
                                 // PM2.5 is above this limit to return to LCDBACKGROUNDCOLOR background when below.
                                 // set to zero to disable
                                 
#define LIMITOFFSET  3           // trigger number of PM's below the limit after measuring above the limitvalue
                                 // This is needed as the SN-GCJA5 output does not show a steady line
                                 // the output is nervous with a lot of peaks and dip's
                                 // else the LCD would change color all the time.
                                 // if a limit = 55 and limitoffset  = 3, the alarm level will increase at 55
                                 // and degrees at 55 - 3 = 52.

#define ONLYONLIMIT false        // only display the results on the LCD display if the PM2_LIMIT is exceeded
                                 // set to false disables this option.
                                 // do NOT select together with ONLYONBUTTON
                                 // make sure to set PM2_LIMITs > 0 (compile will fail)

#define ONLYONBUTTON false       // only display the results on the LCD display if the PM2_LIMITs
                                 // is exceeded OR for LCDTIMEOUT seconds if a button is pushed
                                 // set to false disables this option
                                 // do NOT select together with ONLYONLIMIT
                                 // if PM2_LIMITs are zero the LCD will only display when button is pressed
#if ONLYONBUTTON == true
#define BUTTONINPUT  10         // Digital input where button is connected for ONLYONBUTTON between GND
                                 // is ignored if ONLYONBUTTON is set to false
                                 // Artemis / Apollo3 set as D27
                                 // ESP32, Arduino set as 10

#define LCDTIMEOUT 10            // Number of seconds LCD is displayed after button was pressed
                                 // is ignored if ONLYONBUTTON is set to false
                                  
#endif                          

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
#if SN_SERIALCOMMS != true && SN_I2C_COMMS != true
#error "Select communication to use"
#endif

#if ONLYONLIMIT == true && ONLYONBUTTON == true
#error "you can NOT set BOTH ONLYONLIMIT and ONLYONBUTTON to true"
#endif

#if ONLYONLIMIT == true && ( PM2_LIMITLOW == 0 ||PM2_LIMITHIGH == 0)
#error you MUST set PM2_LIMITLOW and PM2_LIMITHIGH when ONLYONLIMIT to true
#endif

#if SN_SERIALCOMMS == true
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

#if SN_I2C_COMMS == true
// wire settings
#include "SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_Particle_SN-GCJA5
SFE_PARTICLE_SENSOR myAirSensor;
#endif

#include <SerLCD.h> // Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd;         // Initialize the library with default I2C address 0x72

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Panasonic SN-GCJA5 Serial / I2c Example"));
  
  LCDCON.begin();

  // initialize LCD
  lcdinit();
  
#if SN_I2C_COMMS == true
  Wire.begin();

  if (myAirSensor.begin() == false)
  {
    Serial.println(F("The particle sensor did not respond. Please check wiring. Freezing..."));
    lcd.setBacklight(255, 0, 0);    // bright red
    lcd.clear();
    lcd.write("Sensor Error");
    while (1);
  }
  
  Serial.println(F("The particle sensor SN-GCJA5 detected"));
#endif


#if SN_SERIALCOMMS == true
  SenCom.begin(9600, SERIAL_8E1);
  offset = 0;
#endif
 
  Serial.println(F("Sensor started"));
  lcd.clear();
  lcd.write("Sensor Started");
  delay(2000);
}

void loop()
{
  read_all();
  
  // delay for 3 seconds but capture button pressed
  for (int i=0; i < 10; i++){
    delay(300);
    checkButton();
  }
}

// read the values and display
void read_all()
{
  bool SerComplete = false;

#if SN_SERIALCOMMS == true

  offset = 0;
  
  // clear anything pending in the serial buffer
  while (SenCom.available()) SenCom.read();
  
  // wait for new data to arrive (each second)
  delay(1100);
  
  // read data from serial port
  if(SenCom.available()){
    
    while (SenCom.available()){
      data[offset] = SenCom.read();
     
      // check for start of message
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

        SerComplete = true;  // we have valid data
        break;
  
      }

      offset++;             // next position
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

  // if NOT only LCD
  if (! LCDOUTPUTONLY) {
    
    // print result on one line, easy for spreadsheet == true
    if (SpreadSheetLayout) display_spreadSheet(SerComplete);
    
    // print result underneath each other
    else display_line(SerComplete);
  }

  printLCD(SerComplete);
}

//////////////////////////////////////////////////////////////////////
/////////////////////// SERIAL ROUTINES //////////////////////////////
//////////////////////////////////////////////////////////////////////
#if SN_SERIALCOMMS == true
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
  
 #if SN_I2C_COMMS == true 
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

#if SN_SERIALCOMMS == true
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

#if SN_I2C_COMMS == true 
  
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

#if SN_SERIALCOMMS == true
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
  ival += bufto16(SER_SNGCJA5_PCOUNT_10);     // 7.5 - 10 um

  Serial.print(ival);
  
  
#endif  

  Serial.println();
}

//////////////////////////////////////////////////////////////////////
/////////////////////// LCD ROUTINES /////////////////////////////////
//////////////////////////////////////////////////////////////////////

// checks for button pressed to set the LCD on and keep on for
// LCDTIMEOUT seconds after button has been released.
// return true to turn on OR false to turn / stay off.
bool checkButton()
{
#if ONLYONBUTTON == true
  static unsigned long startTime = 0;

  // button pressed ?
  if (! digitalRead(BUTTONINPUT))  startTime = millis();

  // once keep it on for LCDTIMEOUT seconds after release
  if (startTime > 0) {
    if (millis() - startTime < (LCDTIMEOUT*1000))  return true;
    else  startTime = 0;      // reset starttime
  }

  return false;

#endif //ONLYONBUTTON
  return true;
}

// initialize the LCD
void lcdinit()
{
  lcd.begin(LCDCON);
  lcdsetbackground();         // set background

#if ONLYONBUTTON == true
  pinMode(BUTTONINPUT,INPUT_PULLUP);
#endif
}

// set requested background color
void lcdsetbackground()
{

#if ONLYONLIMIT == true
  lcd.setBacklight(0, 0, 0);  // off
  return;
#endif

#if ONLYONBUTTON == true
  if(! checkButton()) {
    lcd.setBacklight(0, 0, 0);  // off
    return;
  }
#endif //ONLYONBUTTON

  switch(LCDBACKGROUNDCOLOR){

    case 2:   // red
      lcd.setBacklight(255, 0, 0); // bright red
      break;
    case 3:   // green
      lcd.setBacklight(0, 255, 0); // bright green
      break;
    case 4:   // blue
      lcd.setBacklight(0, 0, 255); // bright blue
      break;
    case 5:   // off
      lcd.setBacklight(0, 0, 0);
      break;
    case 1:   // white
    default:
      lcd.setBacklight(255, 255, 255); // bright white
  }
}

// print results on LCD
// @parameter dd : true is display new data else on Serial include the no-data indicator
void printLCD(bool dd)
{
  char buf[10];
  static bool limitLowWasSet = false;
  static bool limitHighWasSet = false;

#if SN_I2C_COMMS == true
    float val = myAirSensor.getPM2_5();
#else
    float val = (float) bufto32(SER_SNGCJA5_PM2_5);
#endif

  // change background to red on high limit (if limit was set)
  if (PM2_LIMITHIGH > 0) {

    if (val > PM2_LIMITHIGH) {
      // change once..
      if(! limitHighWasSet){
        lcd.setBacklight(255, 0, 0); // bright red
        limitHighWasSet = true;
      }
    }
    else if (limitHighWasSet){
      // check lower limit 
      if  (val < PM2_LIMITHIGH - LIMITOFFSET) {
        lcd.setBacklight(0, 0, 255); // bright blue
        limitHighWasSet = false;
      }
    }
  }

  // change background on limit (if limit was set)
  if (PM2_LIMITLOW > 0) {

    if ( ! limitHighWasSet ) {
  
      if (val > PM2_LIMITLOW){
        // change once..
        if(! limitLowWasSet) {
          lcd.setBacklight(0, 0, 255); // bright blue
          limitLowWasSet = true;
        }
      }
      else if (limitLowWasSet) {
        // check lower limit 
        if  (val < PM2_LIMITLOW - LIMITOFFSET) {
          lcdsetbackground();           // reset to original request
          limitLowWasSet = false;
        }
      }
    }
  }

// only display if limit has been reached ??
#if ONLYONLIMIT == true
  if(! limitLowWasSet || ! limitLowWasSet) {
    lcd.clear();
    return;
  }
#endif //ONLYONLIMIT

  lcd.clear();
  lcd.write("PM1: PM2: PM10:");
  
  // first try to display based on I2C
#if SN_I2C_COMMS == true
  lcd.setCursor(0, 1);            // pos 0, line 1
  FromFloat(buf, myAirSensor.getPM1_0(),1);
  lcd.write(buf);

  lcd.setCursor(5, 1);            // pos 5, line 1
  FromFloat(buf, myAirSensor.getPM2_5(),1);
  lcd.write(buf);

  lcd.setCursor(10, 1);           // pos 10, line 1
  FromFloat(buf, myAirSensor.getPM10(),1);
  lcd.write(buf);
  
#else  // else use serial info (NO info behind the decimal point)

  // if no new serial data available indicate with .
  if (! dd) {
    lcd.setCursor(15, 0);         // pos 15, line 0
    lcd.write(".");               // display No new data indicator
  }
  
  lcd.setCursor(0, 1);            // pos 0, line 1
  FromFloat(buf, (double) bufto32(SER_SNGCJA5_PM1_0),1);
  lcd.write(buf);

  lcd.setCursor(5, 1);            // pos 5, line 1
  FromFloat(buf, (double) bufto32(SER_SNGCJA5_PM2_5),1);
  lcd.write(buf);

  lcd.setCursor(10, 1);           // pos 10, line 1
  FromFloat(buf, (double) bufto32(SER_SNGCJA5_PM10),1);
  lcd.write(buf);

#endif //SN_I2C_COMMS
}

// This is a workaround as sprintf on Artemis/Apollo3 is not recognizing %f (returns empty)
// based on source print.cpp/ printFloat
int FromFloat(char *buf, double number, uint8_t digits)
{
  char t_buf[10];
  buf[0] = 0x0;

  if (isnan(number)) {
    strcpy(buf,"nan");
    return 3;
  }

  if (isinf(number)) {
    strcpy(buf,"inf");
    return 3;
  }

  if (number > 4294967040.0 || number <-4294967040.0) {
    strcpy(buf,"ovf");
    return 3;
  }

  // Handle negative numbers
  if (number < 0.0)
  {
     strcat(buf,"-");
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;

  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;

  sprintf(t_buf,"%ld", int_part);
  strcat(buf,t_buf);

  if (digits > 0) {

    // Print the decimal point, but only if there are digits beyond
    strcat(buf,".");

    // Extract digits from the remainder one at a time
    while (digits-- > 0)
    {
      remainder *= 10.0;
      unsigned int toPrint = (unsigned int)(remainder);
      sprintf(t_buf,"%d", toPrint);
      strcat(buf,t_buf);
      remainder -= toPrint;
    }
  }

  return (int) strlen(buf);
}
