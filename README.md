# Dual-core-FFT
Performing the Fast Fourier Transform(FFT) on the dual core esp32.
Core0 handles networking related tasks while core1 handles reding sensor data and performing FFT.

# Reqirements

ESP-wroom-32 (or any generic esp32 board for that matter).
A UDP receiver ( clone the code from here: ).
Potentiometer.
