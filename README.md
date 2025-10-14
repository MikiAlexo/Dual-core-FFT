# Dual-core-FFT
Performing the Fast Fourier Transform(FFT) on the dual core esp32.
Core0 handles networking related tasks while core1 handles reding sensor data and performing FFT.

For the networking part i'll be using UPD protocol. why? because i'm lazy to setup HTTP client and i already have a UDP client.

u can find the UPD client here: 

# Reqirements

- ESP-wroom-32 (or any generic esp32 board for that matter).
- A UDP receiver ( clone the code from here: ).
- Potentiometer.
