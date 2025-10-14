#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_adc/adc_oneshot.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#define FFT_SIZE 256
#define UDP_PORT 8888
#define UDP_IP   "192.168.1.100"  // run ipconfig on cmd to find ur's

static const char* TAG = "DualCoreUDP";


float latestFFT[FFT_SIZE];
volatile bool fftReady = false;

// core1
void sensor_fft_task(void *pvParameters){

    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    adc_oneshot_new_unit(&unit_cfg, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_cfg);

    while (1) {
        float sensorData[FFT_SIZE];

        for (int i = 0; i < FFT_SIZE; i++) {
            int raw;
            adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw);
            sensorData[i] = (float)raw;
            esp_rom_delay_us(100);
        }

        // Placeholder FFT, FFT is harder to make than i thought
        for (int i = 0; i < FFT_SIZE; i++) {
            latestFFT[i] = sensorData[i];
        }

        fftReady = true;
        ESP_LOGI(TAG, "FFT computed on Core %d", xPortGetCoreID());

        vTaskDelay(pdMS_TO_TICKS(30000)); // 30s interval
    }
}

//core0
void send_udp_task(void *pvParameters){
    struct sockaddr_in dest_addr;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    dest_addr.sin_addr.s_addr = inet_addr(UDP_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 min

        if (fftReady) {
            char msg[2048];
            int offset = 0;
            for (int i = 0; i < FFT_SIZE; i++) {
                offset += snprintf(msg + offset, sizeof(msg) - offset, "%.2f", latestFFT[i]);
                if (i < FFT_SIZE - 1) offset += snprintf(msg + offset, sizeof(msg) - offset, ",");
            }

            int err = sendto(sock, msg, strlen(msg), 0,
                             (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err < 0) ESP_LOGE(TAG, "Error sending UDP: %d", errno);
            else {
                ESP_LOGI(TAG, "Sent FFT (%d bytes) to %s:%d", err, UDP_IP, UDP_PORT);
                fftReady = false;
            }
        }
    }

    close(sock);
}

void app_main(void){

    
    xTaskCreatePinnedToCore(
        sensor_fft_task,
        "SensorFFTTask",
        8192,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        send_udp_task,
        "SendUDPTask",
        8192,
        NULL,
        1,
        NULL,
        0
    );

    ESP_LOGI(TAG, "Dual-core UDP tasks started");
}

