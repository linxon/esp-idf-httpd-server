#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include <sys/param.h>
#include <esp_netif.h>
#include <esp_eth.h>
#include <esp_event.h>
#include <esp_err.h>
#include <esp_types.h>
#include <nvs_flash.h>

#include "esp_wifi.h"
#include <esp_http_server.h>

static const char *TAG = "hello_server";

esp_err_t post_handler(httpd_req_t *req) {
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            httpd_resp_send_408(req);

        return ESP_FAIL;
    }

    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

esp_err_t get_handler(httpd_req_t *req) {
    const char resp[] = "URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t index_post_uri = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL
};

httpd_uri_t index_get_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting web server on port: %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_get_uri);
        httpd_register_uri_handler(server, &index_post_uri);

        return server;
    }

    ESP_LOGW(TAG, "Error starting web server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server) {
    ESP_LOGI(TAG, "Stopping web server...");
    if (server)
        httpd_stop(server);
}

// httpd_handle_t restart_webserver(httpd_handle_t server) {
//     ESP_LOGI(TAG, "Restart web server...");
//     if (server) {
//         stop_webserver(server);
//         vTaskDelay(500 / portTICK_PERIOD_MS);
//         return start_webserver();
//     }

//     return NULL;
// }

void app_main(void) {
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_conn_cfg = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK
        }
    };

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_conn_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    if (esp_wifi_connect() == ESP_OK)
        start_webserver();
}
