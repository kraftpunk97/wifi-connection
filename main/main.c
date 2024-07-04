#include <string.h>

// FreeRTOS stuff
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// ESP32 stuff
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Socket Programming stuff
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "sdkconfig.h"

// Defines
#define WIFI_SUCCESS 1<<0
#define WIFI_FAILURE 1<<1
#define TCP_SUCCESS 1<<0
#define TCP_FAILURE 1<<1
#define MAX_FAILURE 10

/// ENTER IP ADDRESS OF THE SERVER HERE ///
const char* ip_address = "";
const int port = 12345;

static const char* TAG = "WIFI";

static EventGroupHandle_t wifi_event_group;

static int s_retry_num = 0;

// Handler for WiFi events
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to Access Point...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_FAILURE) {
            ESP_LOGI(TAG, "Reconnecting to Access Point....");
            esp_wifi_connect();
            s_retry_num += 1;
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
        }
    }
}

// What to do when you have gotten an IP
static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }
}

// connect to the wifi and return the result
esp_err_t connect_wifi() {
    int status = WIFI_FAILURE;

    // init the ESP32 Network Interface
    ESP_ERROR_CHECK(esp_netif_init());

    // init default esp event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // create wifi station in the wifi driver
    esp_netif_create_default_wifi_sta();

    // setup wifi station in the wifi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Begin event loop handling
    wifi_event_group = xEventGroupCreate();


    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &wifi_handler_event_instance)); // TODO: Tinker with this...

    esp_event_handler_instance_t got_ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL,
                                                        &got_ip_event_instance)); // TODO: Tinker with this...

    wifi_config_t wifi_config = {
        .sta = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
        .threshold = {
            .rssi = -127,
        },
        .pmf_cfg = {
            .capable = true,
        }
    }
    };

    if (strlen(CONFIG_WIFI_PASSWORD) != 0) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    // set the WiFi controller to be a station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // set the WiFi config
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "STA Init complete");

    // start the WiFi driver
    ESP_ERROR_CHECK(esp_wifi_start());

    // Now we wait for a connection
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_SUCCESS | WIFI_FAILURE,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    
    // xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened.
    if (bits & WIFI_SUCCESS) {
        ESP_LOGI(TAG, "Connected to an access point.");
        status = WIFI_SUCCESS;
    } else if (bits & WIFI_FAILURE) {
        ESP_LOGI(TAG, "Failed to connect.");
        status = WIFI_FAILURE;
    } else {
        ESP_LOGI(TAG, "Unexpected event occured.");
        status = WIFI_FAILURE;
    }

    // Unregister event handlers and event group
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    vEventGroupDelete(wifi_event_group);
    
    return status;
}

esp_err_t connect_tcp_server() {
    struct sockaddr_in server_info = {0};
    char read_buffer[200] = {0};

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(12345);
    if (inet_pton(AF_INET, "172.28.242.211", &server_info.sin_addr) == -1) {
        ESP_LOGE(TAG, "Invalid host");
        return TCP_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return TCP_FAILURE;
    }
    if (connect(sock, (struct sockaddr *) &server_info, sizeof(server_info)) != 0) {
        ESP_LOGE(TAG, "Failed to connect ");
        close(sock);
        return TCP_FAILURE;
    }

    ESP_LOGI(TAG, "Connected to TCP server");
    
    while (true) {
        bzero(read_buffer, sizeof(read_buffer));
        int r = read(sock, read_buffer, sizeof(read_buffer)-1);
        for (int i=0; i<r; i++) {
            putchar(read_buffer[i]);
        }

        if (strcmp(read_buffer, "HELLO\n") == 0) {
    	    ESP_LOGI(TAG, "WE DID IT!");
            break;
        }
    }

    return TCP_SUCCESS;
}

void app_main(void) {
    esp_err_t status = WIFI_FAILURE;

    // initialize storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // connect to wireless access point
    status = connect_wifi();
    if (status != WIFI_SUCCESS) {
        ESP_LOGE(TAG, "Failed to associate to access point. Dying...");
        return;
    }
    
    status = connect_tcp_server();
    if (status != TCP_SUCCESS) {
        ESP_LOGE(TAG, "Failed to connect to remote server. Dying...");
        return;
    }

}