# ESP32 Weather
Personal weather station. Tested with ESP-IDF release v5.5.2.

## Components

- [SparkFun Weather MicroMod carrier][weather-micromod]
- [SparkFun SAM-M8Q GPS breakout][gps-breakout]
- [SparkFun weather meters][weather-meters]
- [SparkFun ESP32 MicroMod processor][esp32-micromod]

## Building

The Weather MicroMod and SAM-M8Q should have their UART headers populated.

I can't seem to get the SAM-M8Q's Qwiic I2C working with the ESP32, so I used
the SAM-M8Q's FTDI UART header to communicate with the ESP32.

Flash the ESP32 in the Weather Carrier using its USB-C port, it works usually.

[weather-micromod]: https://www.sparkfun.com/products/16794
[gps-breakout]: https://www.sparkfun.com/products/15210
[weather-meters]: https://www.sparkfun.com/products/15901
[esp32-micromod]: https://www.sparkfun.com/products/16781
