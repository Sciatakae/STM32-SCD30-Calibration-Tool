# STM32-SCD30-Calibration-Tool

I decided to upload this code is to provide STM32 examples for other beginners to reference.

This project is a good example if you are learning about I2C and/or UART on an STM32 microcontroller; or if you just need to calibrate your sensor on an STM32 microcontroller.

- On boot, a menu in the terminal appears and you can start/stop streaming sensor data with ASC calibration enabled/disabled at your discression.

- When the calibration starts; leave the device on for 7 days with 1 hour a day of "fresh air"



Microcontroller:  Nucleo-L476RG

Software: STM32CubeMX & Visual Studio Code

As of July 16, 2026

- I haven't left the device running for the 7 days needed to auto calibrate, will share results when I do.

- Currently no FRC calibration option/feature as I dont have a second reference to calibrate this sensor, but I will update the code when I make progress in this route. 

Official Sensiron SCD30 driver: https://github.com/Sensirion/embedded-i2c-scd30

Sensiron SCD30 datasheet: https://sensirion.com/media/documents/4EAF6AF8/61652C3C/Sensirion_CO2_Sensors_SCD30_Datasheet.pdf
