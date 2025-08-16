#include "influx_logger.h"
#include <stdio.h>
#include <curl/curl.h>

// --- InfluxDB Configuration ---
static const char* INFLUX_URL = "http://localhost:8086";
static const char* INFLUX_ORG = "Home";
static const char* INFLUX_BUCKET = "rpi-home";
static const char* INFLUX_TOKEN = "ZpQrOazYyAXyLwWlFlJqWdQGXAMrkye18BqQGbh_gC1nvIQerO98ebGkA2Bkk8cExnx--F6azcCz2KcUKD-_aQ==";

static CURL *curl_handle = NULL;
static struct curl_slist *curl_headers = NULL;

int influx_logger_init(void) {

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();
    if (!curl_handle) {
        fprintf(stderr, "curl_easy_init() failed\n");
        return -1;
    }
    char api_url[256];
    snprintf(api_url, sizeof(api_url), "%s/api/v2/write?org=%s&bucket=%s&precision=s",
             INFLUX_URL, INFLUX_ORG, INFLUX_BUCKET);

    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s", INFLUX_TOKEN);

    curl_headers = curl_slist_append(curl_headers, auth_header);
    curl_headers = curl_slist_append(curl_headers, "Content-Type: text/plain; charset=utf-8");
    curl_headers = curl_slist_append(curl_headers, "Accept: application/json");

    curl_easy_setopt(curl_handle, CURLOPT_URL, api_url);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, curl_headers);

    printf("Influx logger initialized and configured for writing.\n");
    return 0; // Success
}

void influx_logger_send_scd41_data(const scd41_measurement_t* measurement) {
    if (!curl_handle) {
        fprintf(stderr, "Influx logger not initialized.\n");
        return;
    }

    char post_data[256];
    // Format the data in InfluxDB Line Protocol format
    snprintf(post_data, sizeof(post_data), "scd41_readings,location=living_room co2=%d,temp=%.2f,humidity=%.2f",
             measurement->co2_ppm, measurement->temperature_c, measurement->humidity_rh);

    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);

    CURLcode res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("Data successfully sent to InfluxDB.\n");
    }
}

void influx_logger_cleanup(void) {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
        curl_slist_free_all(curl_headers);
    }
    curl_global_cleanup();
    printf("Influx logger cleaned up.\n");
}
