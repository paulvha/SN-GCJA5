SparkFun Particle Sensor SN-GCJA5 Arduino Library
========================================

![SparkFun Panasonic SN-GCJA5 Particle Sensor](https://cdn.sparkfun.com//assets/parts/1/6/0/9/3/17123-Particle_Sensor_-_SN-GCJA5-01.jpg)

[*SparkFun Panasonic SN-GCJA5 Particle Sensor (SPX-17123)*](https://www.sparkfun.com/products/17123)

The Panasonic SN-GCJA5 is a highly accurate and easy to use particle matter detector. Great for detecting PM1.0, PM2.5, and PM10 sized particles. The only downside to this device is that it requires 5V (presumably to run the fan) but 3.3V for the I2C interface pins.

## versioning

 BASED ON THE ORIGINAL [SPARKFUN LIBRARY](https://github.com/sparkfun/SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library)
 8 November 2020 / paulvha
 * added product documentation in [extras](extras)
 * added [deep-dive analyses](extras/SN-GCJA5.odt) on the sensor
 * examples added to also read the SN-GCJA5 serial
 * added example10, 11, 12 and 13
 * added additional call in library for TestReg

 12 November 2020 / paulvha
 * Changed the way I2C reads are performed to improve stability
 * For Raspberry Pi there is also a [library](https://github.com/paulvha/SN-GCJA5_on_raspberry)
 * Documentation has been updated with new learnings

## Repository Contents

- **/examples** - Example sketches for the library (.ino). Run these from the Arduino IDE.
- **/src** - Source files for the library (.cpp, .h).
- **keywords.txt** - Keywords from this library that will be highlighted in the Arduino IDE.
- **library.properties** - General library properties for the Arduino package manager.

## Products That Use This Library

- [Particle Sensor - SN-GCJA5 (SPX-17123)](https://www.sparkfun.com/products/17123)

## Documentation

- **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - Basic information on how to install an Arduino library.

## License Information

This product is _**open source**_!

Please use, reuse, and modify these files as you see fit.
Please maintain the attribution and release anything derivative under the same license.

Distributed as-is; no warranty is given.
