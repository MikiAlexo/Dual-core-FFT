#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_adc/adc_oneshot.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "fft_wA.h"

#define FFT_SIZE 256
#define UDP_PORT 8888
#define UDP_IP   "192.168.4.2"  

#define WIFI_SSID "ESP32_AP"      
#define WIFI_PASS "12345678"
#define WIFI_CHANNEL 1
#define MAX_CONN 2  

static const char* TAG = "DualCoreUDP";

float latestFFT[FFT_SIZE];
volatile bool fftReady = false;
portMUX_TYPE fft_mux = portMUX_INITIALIZER_UNLOCKED;  // for thread safety


static void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data){
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Client connected: "MACSTR, MAC2STR(event->mac));
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Client disconnected: "MACSTR, MAC2STR(event->mac));
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP started. SSID:%s PASS:%s", WIFI_SSID, WIFI_PASS);
}

void sensor_fft_task(void *pvParameters){
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    adc_oneshot_new_unit(&unit_cfg, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_cfg);

    float sensorData[FFT_SIZE];
    float fftOutput[FFT_SIZE]; 

    while (1) {
        
        for (int i = 0; i < FFT_SIZE; i++) {
            int raw;
            adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw);
            sensorData[i] = (float)raw;
            esp_rom_delay_us(100);
        }

        fft_compute(sensorData, fftOutput, FFT_SIZE);

       
        taskENTER_CRITICAL(&fft_mux);
        memcpy(latestFFT, fftOutput, sizeof(latestFFT));
        fftReady = true;
        taskEXIT_CRITICAL(&fft_mux);

        ESP_LOGI(TAG, "FFT computed on Core %d", xPortGetCoreID());

        vTaskDelay(pdMS_TO_TICKS(30000)); 
    }
}

void send_udp_task(void *pvParameters){
    struct sockaddr_in dest_addr;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed: %d", errno);
        vTaskDelete(NULL);
        return;
    }

    dest_addr.sin_addr.s_addr = inet_addr(UDP_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(300000));  

        if (fftReady) {
            char msg[2048];
            int offset = 0;

            taskENTER_CRITICAL(&fft_mux);
            for (int i = 0; i < FFT_SIZE; i++) {
                offset += snprintf(msg + offset, sizeof(msg) - offset, "%.2f", latestFFT[i]);
                if (i < FFT_SIZE - 1) offset += snprintf(msg + offset, sizeof(msg) - offset, ",");
            }
            fftReady = false;
            taskEXIT_CRITICAL(&fft_mux);

            int err = sendto(sock, msg, strlen(msg), 0,
                             (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err < 0) {
                ESP_LOGE(TAG, "UDP send failed: %d", errno);
            } else {
                ESP_LOGI(TAG, "Sent FFT (%d bytes) to %s:%d", err, UDP_IP, UDP_PORT);
            }
        }
    }

    close(sock);
}


void app_main(void){
 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_softap();

    xTaskCreatePinnedToCore(sensor_fft_task, "SensorFFTTask", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(send_udp_task,   "SendUDPTask",   8192, NULL, 1, NULL, 0);

    ESP_LOGI(TAG, "Dual-core UDP tasks started");
}
