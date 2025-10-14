# Dual-core-FFT
Performing the Fast Fourier Transform(FFT) on the dual core esp32.
Core0 handles networking related tasks while core1 handles reding sensor data and performing FFT.

For the networking part i'll be using UPD protocol. why? because i'm lazy to setup HTTP client and i already have a UDP client.

u can find the UPD client here: 

# Reqirements

- ESP-wroom-32 (or any generic esp32 board for that matter).
- A UDP receiver ( clone the code from here: ).
- Potentiometer.

  ## Wiring
  - Connect the the VCC( any of the two pins on the edge) of the pot to 3.3V of esp and the - to GND of esp
    the signal of the pot( the middle one) to ADC1-CH6( GPIO 34) of esp.

  here's the pinouts of the esp32-wroom
  
![adc-pins-esp32-f](https://github.com/user-attachments/assets/9e49eb00-d256-40f6-aa3b-319cf18c96fe)
