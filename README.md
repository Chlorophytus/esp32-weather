# ESP32 Weather
Personal weather station. Tested with ESP-IDF release v6.0.x.

## To Do
- Interface with SD card (5 is SPI_CS, 18 is SPI_SCK, 23 is COPI (input to SD card), 19 is CIPO (output from SD card))

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
