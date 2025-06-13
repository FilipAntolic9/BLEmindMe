/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */



/****************************************************************************
*
* This file is for iBeacon demo. It supports both iBeacon sender and receiver
* which is distinguished by macros IBEACON_SENDER and IBEACON_RECEIVER,
*
* iBeacon is a trademark of Apple Inc. Before building devices which use iBeacon technology,
* visit https://developer.apple.com/ibeacon/ to obtain a license.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_ibeacon_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "protocol_examples_common.h"
#include "example_common_private.h"

#include <cJSON.h>

#include "esp_wifi.h"

#include "driver/gpio.h"
#include "esp_private/esp_clk.h"
#include "driver/mcpwm_cap.h"

/* --------------------------------PARAMETRI KOJI SE OPCIONALNO MOGU PRILAGODITI VLASTITOJ POTREBI----------------------------------------------------------------------- */
#define MAX_LEN_MQTT_USER_CREDENTIALS 64

#define HCSR04_MAX_READINGS_AVERAGE 64      // Broj mjerenja koje se koristi za izračunavanje prosječne udaljenosti
#define HCSR04_WAIT_BETWEEN_READINGS_MS 500 // 500 ms -> MAX_READINGS_AVERAGE * HCSR04_WAIT_BETWEEN_READINGS_MS = vrijeme "zagrijavanja" senzora
#define HCSR04_DISTANCE_DELTA_TRIGGER 20.0f // 20 cm - Vrijednost pomaka udaljenosti iznad koje se senzor smatra da je korisnik ušao ili izašao iz prostorije
#define HCSR04_TRIGGER_COOLDOWN_CYCLES 20   // 20 cycles * 500 ms = 10 seconds cooldown - Vrijeme čekanja nakon što se detektira promjena udaljenosti prije nego što se ponovno može detektirati promjena udaljenosti

#define IBEACON_DISTANCE_THRESHOLD 8.0f     // 8 meters - Vrijednost udaljenosti ispod koje se smatra da je korisnik prisutan u prostoriji
/* -------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* --------------------------------PARAMETRI KOJE JE OBAVEZNO PRILAGODITI VLASTITOJ POTREBI----------------------------------------------------------------------- */
#define CONFIG_WIFI_SSID ""                        // Change to your Wi-Fi SSID <--------------------------------------------------------------------------------------------------------
#define CONFIG_WIFI_PASSWORD ""             // Change to your Wi-Fi password <----------------------------------------------------------------------------------------------------

#define CONFIG_MQTT_BROKER_URL "mqtt://161.53.133.253:1883" // Change to your MQTT broker URL <---------------------------------------------------------------------------------------------------
#define CONFIG_MQTT_PROXY_USERNAME "proxy"                  // Change to your MQTT proxy username <-----------------------------------------------------------------------------------------------
#define CONFIG_MQTT_PROXY_PASSWORD "proxy"                  // Change to your MQTT proxy password <-----------------------------------------------------------------------------------------------
#define CONFIG_MQTT_PROXY_ID "proxy"                        // Change to your MQTT proxy ID <-----------------------------------------------------------------------------------------------------
#define CONFIG_MQTT_PROXY_CON_AUTH "{username: \"iva.ivic@gmail.com\", password: \"ivaivic\", customerId: \"ce4757a0-414b-11f0-a544-db21b46190ed\", espId: \"0c75afe0-43e0-11f0-a544-db21b46190ed\"}"
// Change to your MQTT proxy connection authentication JSON values ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^                                                      
/* -------------------------------------------------------------------------------------------------------------------------------------------------------------- */
#define LED_GPIO            GPIO_NUM_20
#define HC_SR04_TRIG_GPIO   GPIO_NUM_10
#define HC_SR04_ECHO_GPIO   GPIO_NUM_11

int mqtt_proxy_sub_msg_id, mqtt_proxy_cred_rec;

esp_mqtt_client_handle_t globalClient;

static const char* TAG = "BLEMINDME_1";

static const char* BASE_TOPIC = "v1/devices/esp/";

extern esp_ble_ibeacon_vendor_t vendor_config;

bool proxy_connected = false;
bool user_in_room = false;

char mqtt_user_userid[MAX_LEN_MQTT_USER_CREDENTIALS+1] = {0};
char mqtt_user_password[MAX_LEN_MQTT_USER_CREDENTIALS+1] = {0};
char mqtt_user_username[MAX_LEN_MQTT_USER_CREDENTIALS+1] = {0};

/*
Topics:
v1/devices/esp/<username>/stateUpdate   <--- iBeacon data updates   [SENDING]
v1/devices/esp/<username>/userLeft      <--- user left the room     [SENDING]
v1/devices/esp/attributes/<username>    <--- light state updates    [RECEIVING]
*/

char mqtt_user_items_update_topic[128] = {0};
char mqtt_user_presence_topic[128] = {0};
char mqtt_user_light_state_topic[128] = {0};
char mqtt_user_light_state_json[128] = {0};

typedef struct{
    char name[MAX_LEN_MQTT_USER_CREDENTIALS+1];
    char id[MAX_LEN_MQTT_USER_CREDENTIALS+1];
    char label[MAX_LEN_MQTT_USER_CREDENTIALS+1];
    double distance;
    bool present;
} device_t;

device_t* device_list = NULL;
static int device_list_size = 0;

esp_mqtt_client_config_t mqtt5_user_cfg = {
                            .broker.address.uri = CONFIG_MQTT_BROKER_URL,
                            .session.protocol_ver = MQTT_PROTOCOL_V_5,
                            .network.reconnect_timeout_ms = 5000,
                            .credentials.username = &mqtt_user_username[0],
                            .credentials.authentication.password = &mqtt_user_password[0],
                            .credentials.client_id = &mqtt_user_userid[0],
                        };

static esp_mqtt5_publish_property_config_t publish_property = {
    .payload_format_indicator = 1,
    .message_expiry_interval = 1000,
    .topic_alias = 0,
    .response_topic = "v1/devices/me/attributes",
    .correlation_data = "123456",
    .correlation_data_len = 6,
};

static esp_mqtt5_subscribe_property_config_t subscribe_property = {
    .subscribe_id = 25555,
    .no_local_flag = false,
    .retain_as_published_flag = false,
    .retain_handle = 0,
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_subscribe_property_config_t subscribe1_property = {
    .subscribe_id = 25555,
    .no_local_flag = true,
    .retain_as_published_flag = false,
    .retain_handle = 0,
};

static esp_mqtt5_unsubscribe_property_config_t unsubscribe_property = {
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_disconnect_property_config_t disconnect_property = {
    .session_expiry_interval = 60,
    .disconnect_reason = 0,
};

static esp_mqtt5_user_property_item_t user_property_arr[] = {
        {"board", "esp32"},
    };

#define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)

///Declare static functions
/* -------------------------------------------GPIO------------------------------------------------------*/

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring LED output!");
    gpio_reset_pin(LED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

void set_led_state(bool state)
{
    gpio_set_level(LED_GPIO, state ? 1 : 0);
}

/* -------------------------------------------HC-SR04---------------------------------------------------*/
void hcsr04_send_value(void){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "hasLeft", user_in_room);

    char *json_string = cJSON_Print(root);

    ESP_LOGI(TAG, "Sending user presence update: %s", json_string);
    esp_mqtt5_client_set_user_property(&publish_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_publish_property(globalClient, &publish_property);
    // Publish the JSON string to the user presence topic
    esp_mqtt_client_publish(globalClient, mqtt_user_presence_topic, json_string, 0, 1, 1);

    esp_mqtt5_client_delete_user_property(publish_property.user_property);
    publish_property.user_property = NULL;

    // free the JSON string and cJSON object
    cJSON_free(json_string);
    cJSON_Delete(root);
}

void hcsr04_save_reading(float distance){
    static float average_distance = 0.0f;
    static int reading_count = 0;
    static uint8_t trigger_cooldown = 0;
    
    if(reading_count < HCSR04_MAX_READINGS_AVERAGE){
        // Rekurzivna definicija za prosjek
        if(reading_count == 0){
            average_distance = distance; // Prvo mjerenje postavlja prosjek
            reading_count++;
            return;
        }
        float alpha = (float)reading_count / (reading_count + 1);
        average_distance = alpha * average_distance + (1 - alpha) * distance;
        reading_count++;
        ESP_LOGI(TAG, "New reading added: %.2fcm, average distance: %.2fcm", distance, average_distance);
    }else{
        if(distance < average_distance-HCSR04_DISTANCE_DELTA_TRIGGER && trigger_cooldown > HCSR04_TRIGGER_COOLDOWN_CYCLES){
            ESP_LOGI(TAG, "Distance decreased significantly from %.2fcm average to %.2fcm, sending update", average_distance, distance);
            // Send update to MQTT
            user_in_room = !user_in_room; // Toggle user presence state

            hcsr04_send_value();

            trigger_cooldown = 0; // reset cooldown
        }else if(trigger_cooldown <= HCSR04_TRIGGER_COOLDOWN_CYCLES){
            trigger_cooldown++;
        }
    }
}

static bool hc_sr04_echo_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    static uint32_t cap_val_begin_of_sample = 0;
    static uint32_t cap_val_end_of_sample = 0;
    TaskHandle_t task_to_notify = (TaskHandle_t)user_data;
    BaseType_t high_task_wakeup = pdFALSE;

    //calculate the interval in the ISR,
    //so that the interval will be always correct even when capture_queue is not handled in time and overflow.
    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        // store the timestamp when pos edge is detected
        cap_val_begin_of_sample = edata->cap_value;
        
        cap_val_end_of_sample = cap_val_begin_of_sample;
    } else {
        cap_val_end_of_sample = edata->cap_value;
        uint32_t tof_ticks = cap_val_end_of_sample - cap_val_begin_of_sample;

        // notify the task to calculate the distance
        xTaskNotifyFromISR(task_to_notify, tof_ticks, eSetValueWithOverwrite, &high_task_wakeup);
    }

    return high_task_wakeup == pdTRUE;
}

static void hcsr04_init(void)
{
    ESP_LOGI(TAG, "Install capture timer");
    mcpwm_cap_timer_handle_t cap_timer = NULL;
    mcpwm_capture_timer_config_t cap_conf = {
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &cap_timer));

    ESP_LOGI(TAG, "Install capture channel");
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = HC_SR04_ECHO_GPIO,
        .prescale = 1,
        // capture on both edge
        .flags.neg_edge = true,
        .flags.pos_edge = true,
        // pull up internally
        .flags.pull_up = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &cap_ch_conf, &cap_chan));

    ESP_LOGI(TAG, "Register capture callback");
    TaskHandle_t cur_task = xTaskGetCurrentTaskHandle();
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = hc_sr04_echo_callback,
    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, cur_task));

    ESP_LOGI(TAG, "Enable capture channel");
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));

    ESP_LOGI(TAG, "Configure Trig pin");
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << HC_SR04_TRIG_GPIO,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    // drive low by default
    ESP_ERROR_CHECK(gpio_set_level(HC_SR04_TRIG_GPIO, 0));

    ESP_LOGI(TAG, "Enable and start capture timer");
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));
}

/**
 * @brief generate single pulse on Trig pin to start a new sample
 */
static void gen_trig_output(void)
{
    gpio_set_level(HC_SR04_TRIG_GPIO, 1); // set high
    esp_rom_delay_us(10);
    gpio_set_level(HC_SR04_TRIG_GPIO, 0); // set low
}

void hcsr04_main_loop(void *arguments){
    configASSERT( ( ( uint32_t ) arguments ) == 1 );

    ESP_LOGI(TAG, "HC-SR04 initialization started");
    hcsr04_init();
    ESP_LOGI(TAG, "HC-SR04 sensor initialized");

    uint32_t tof_ticks;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(HCSR04_WAIT_BETWEEN_READINGS_MS));
        // trigger the sensor to start a new sample
        ESP_LOGI(TAG, "--------HC-SR04----------");
        // ESP_LOGI(TAG, "Triggering HC-SR04 sensor");
        gen_trig_output();
        // wait for echo done signal
        if (xTaskNotifyWait(0x00, ULONG_MAX, &tof_ticks, pdMS_TO_TICKS(1000)) == pdTRUE) {
            // ESP_LOGI(TAG, "Received echo signal, tof_ticks = %ld", (long)tof_ticks);
            float pulse_width_us = tof_ticks * (1000000.0 / 80000000.0);    // 80 MHz because the clk source is set to default which is 160 MHz and prescaler to 1 so the source is devided by 2
            // APB frequency was used to calculate the pulse width but it is not used to generate the pulse so it is not accurate
            // ESP_LOGI(TAG, "APB clock frequency: %dHz", esp_clk_apb_freq());
            // ESP_LOGI(TAG, "Pulse width: %.2fus", pulse_width_us);

            if (pulse_width_us > 35000) {
                // out of range
                ESP_LOGI(TAG, "Out of range, pulse width too long: %.2fus", pulse_width_us);
                continue;
            }
            // convert the pulse width into measure distance
            float distance = (float) pulse_width_us / 58;
            ESP_LOGI(TAG, "Measured distance: %.2fcm", distance);
            // save the reading
            hcsr04_save_reading(distance);
        }
        ESP_LOGI(TAG, "-------------------------");
        
    }
}
/* -------------------------------------------BLE------------------------------------------------------*/
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
#if (IBEACON_MODE == IBEACON_SENDER)
        esp_ble_gap_start_advertising(&ble_adv_params);
#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
#if (IBEACON_MODE == IBEACON_RECEIVER)
        //the unit of the duration is second, 0 means scan permanently
        uint32_t duration = 0;
        esp_ble_gap_start_scanning(duration);
#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Scanning start failed, error %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Scanning start successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //adv start complete event to indicate adv start successfully or failed
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed, error %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Advertising start successfully");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            /* Search for BLE iBeacon Packet */
            if (esp_ble_is_ibeacon_packet(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len)){
                esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t*)(scan_result->scan_rst.ble_adv);
                ESP_LOGI(TAG, "----------iBeacon Found----------");
                ESP_LOGI(TAG, "Device address: "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda));
                char device_address[18] = {0};
                snprintf(device_address, sizeof(device_address), ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(scan_result->scan_rst.bda));
                // ESP_LOG_BUFFER_HEX("IBEACON_DEMO: Proximity UUID", ibeacon_data->ibeacon_vendor.proximity_uuid, ESP_UUID_LEN_128);

                // uint16_t major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
                // uint16_t minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
                // ESP_LOGI(TAG, "Major: 0x%04x (%d)", major, major);
                // ESP_LOGI(TAG, "Minor: 0x%04x (%d)", minor, minor);
                // ESP_LOGI(TAG, "Measured power (RSSI at a 1m distance): %d dBm", ibeacon_data->ibeacon_vendor.measured_power);
                // ESP_LOGI(TAG, "RSSI of packet: %d dbm", scan_result->scan_rst.rssi);
                
                float distance = pow(10.0f, ((float)ibeacon_data->ibeacon_vendor.measured_power - (float)scan_result->scan_rst.rssi) / 20.0f);
                // float distance = (float)10 ^ ((ibeacon_data->ibeacon_vendor.measured_power - scan_result->scan_rst.rssi) / 20);
                ESP_LOGI(TAG, "iBEACON Estimated distance: %.2f m", distance);
                // ESP_LOGI(TAG, "Devices: %d", device_list_size);
                if(device_list_size > 0){
                    for(int i = 0; i < device_list_size; i++){
                        if(strncmp(device_list[i].label, device_address, MAX_LEN_MQTT_USER_CREDENTIALS+1) == 0){
                            // Device is present in the room
                            ESP_LOGI(TAG, "Device %s (%s) is detected.", device_list[i].name, device_list[i].label);
                            device_list[i].distance = distance;
                            if(distance < IBEACON_DISTANCE_THRESHOLD){
                            // Device is present in the room
                                device_list[i].present = true;
                                ESP_LOGI(TAG, "Device %s (%s) is present in the room.", device_list[i].name, device_list[i].label);
                            }else{
                                // Device is not present in the room
                                device_list[i].present = false;
                                ESP_LOGI(TAG, "Device %s (%s) is too far away", device_list[i].name, device_list[i].label);
                            }
                        }
                    }
                }
                ESP_LOGI(TAG, "----------------------------------");
            }
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "Scanning stop failed, error %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(TAG, "Scanning stop successfully");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "Advertising stop failed, error %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(TAG, "Advertising stop successfully");
        }
        break;

    default:
        break;
    }
}


void ble_ibeacon_appRegister(void)
{
    esp_err_t status;

    ESP_LOGI(TAG, "register callback");

    //register the scan callback function to the gap module
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(status));
        return;
    }

}

void ble_ibeacon_init(void)
{
    esp_bluedroid_init();
    esp_bluedroid_enable();
    ble_ibeacon_appRegister();
}

/* -------------------------------------------MQTT------------------------------------------------------*/

void assamble_mqtt_user_topics(char *username)
{
    snprintf(mqtt_user_items_update_topic, sizeof(mqtt_user_items_update_topic), "%s%s%s", BASE_TOPIC, username, "/stateUpdate");
    snprintf(mqtt_user_presence_topic, sizeof(mqtt_user_presence_topic), "%s%s%s", BASE_TOPIC, username, "/userLeft");
    snprintf(mqtt_user_light_state_topic, sizeof(mqtt_user_light_state_topic), "%s%s", "v1/devices/me/", "attributes");
    snprintf(mqtt_user_light_state_json, sizeof(mqtt_user_light_state_json), "%s_%s", "lightState", username);
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void print_user_property(mqtt5_user_property_handle_t user_property)
{
    if (user_property) {
        uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
        if (count) {
            esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
            if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                for (int i = 0; i < count; i ++) {
                    esp_mqtt5_user_property_item_t *t = &item[i];
                    ESP_LOGI(TAG, "key is %s, value is %s", t->key, t->value);
                    free((char *)t->key);
                    free((char *)t->value);
                }
            }
            free(item);
        }
    }
}

/*
 * @brief Event handler registered to receive MQTT events while connected to proxy
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt5_proxy_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        print_user_property(event->property->user_property);

        esp_mqtt5_client_set_user_property(&subscribe1_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_subscribe_property(client, &subscribe1_property);
        msg_id = esp_mqtt_client_subscribe(client, "v1/devices/me/attributes", 1);
        mqtt_proxy_sub_msg_id = msg_id;
        esp_mqtt5_client_delete_user_property(subscribe_property.user_property);
        subscribe_property.user_property = NULL;
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        if (event->msg_id == mqtt_proxy_sub_msg_id) {
            ESP_LOGI(TAG, "Subscribed to v1/devices/me/attributes, sending credentials");
            esp_mqtt5_client_set_user_property(&publish_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
            esp_mqtt5_client_set_publish_property(client, &publish_property);
            msg_id = esp_mqtt_client_publish(client, "v1/devices/me/attributes", CONFIG_MQTT_PROXY_CON_AUTH, 0, 1, 1);
            esp_mqtt5_client_delete_user_property(publish_property.user_property);
            publish_property.user_property = NULL;
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        }
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "---MQTT_EVENT_DATA---");
        print_user_property(event->property->user_property);
        ESP_LOGI(TAG, "payload_format_indicator is %d", event->property->payload_format_indicator);
        ESP_LOGI(TAG, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
        ESP_LOGI(TAG, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
        ESP_LOGI(TAG, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        ESP_LOGI(TAG, "msg_id=%d", event->msg_id);
        ESP_LOGI(TAG, "---------------------");

        if(strncmp(event->topic, (char *)"v1/devices/me/attributes", 25) == 0) {
            cJSON *root = cJSON_Parse(event->data);
            if (root == NULL) {
                ESP_LOGE(TAG, "Failed to parse JSON data");
            } else {
                cJSON *credentials = cJSON_GetObjectItem(root, "credentials");
                if (credentials != NULL) {
                    cJSON *username = cJSON_GetObjectItem(credentials, "username");
                    cJSON *password = cJSON_GetObjectItem(credentials, "password");
                    cJSON *clientId = cJSON_GetObjectItem(credentials, "clientId");
                    
                    if (username && password && clientId) {
                        ESP_LOGI(TAG, "Received credentials: %s / %s / %s", clientId->valuestring, username->valuestring, password->valuestring);
                        strncpy(&mqtt_user_username[0], username->valuestring, MAX_LEN_MQTT_USER_CREDENTIALS);
                        strncpy(&mqtt_user_password[0], password->valuestring, MAX_LEN_MQTT_USER_CREDENTIALS);
                        strncpy(&mqtt_user_userid[0], clientId->valuestring, MAX_LEN_MQTT_USER_CREDENTIALS);

                        proxy_connected = true; // Set the flag to true when credentials are received
                        ESP_LOGI(TAG, "Proxy connection established, credentials received. Starting relogin...");
                    }
                }
                cJSON *devices = cJSON_GetObjectItem(root, "devices");
                if (devices != NULL) {
                    int device_count = cJSON_GetArraySize(devices);
                    ESP_LOGI(TAG, "Found %d devices", device_count);
                    if (device_list != NULL) {
                        free(device_list);
                    }
                    device_list = malloc(device_count * sizeof(device_t));
                    device_list_size = device_count;
                    for (int i = 0; i < device_count; i++) {
                        cJSON *device = cJSON_GetArrayItem(devices, i);
                        if (device != NULL) {
                            cJSON *name = cJSON_GetObjectItem(device, "name");
                            cJSON *id = cJSON_GetObjectItem(device, "id");
                            cJSON *label = cJSON_GetObjectItem(device, "label");
                            if (name && id && label) {
                                strncpy(device_list[i].name, name->valuestring, MAX_LEN_MQTT_USER_CREDENTIALS);
                                strncpy(device_list[i].id, id->valuestring, MAX_LEN_MQTT_USER_CREDENTIALS);
                                strncpy(device_list[i].label, label->valuestring, MAX_LEN_MQTT_USER_CREDENTIALS);
                                device_list[i].distance = 0.0f; // Initialize distance
                                device_list[i].present = false; // Initialize presence state

                                ESP_LOGI(TAG, "Device %d: %s / %s / %s", i+1, device_list[i].name, device_list[i].id, device_list[i].label);
                            }
                        }
                    }
                }
                cJSON_Delete(root);
            }
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        print_user_property(event->property->user_property);
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/*
 * @brief Event handler registered to receive MQTT events while in user mode
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt5_user_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        print_user_property(event->property->user_property);
        
        esp_mqtt5_client_set_user_property(&subscribe1_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_subscribe_property(client, &subscribe1_property);
        msg_id = esp_mqtt_client_subscribe(client, &mqtt_user_light_state_topic[0], 1);
        esp_mqtt5_client_delete_user_property(subscribe1_property.user_property);
        subscribe1_property.user_property = NULL;
        ESP_LOGI(TAG, "sent subscribe to '%s' successful, msg_id=%d", mqtt_user_light_state_topic, msg_id);

        hcsr04_send_value(); // Send initial presence update

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);

        ESP_LOGI(TAG, "Subscribed");

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "---MQTT_EVENT_DATA---");
        print_user_property(event->property->user_property);
        ESP_LOGI(TAG, "payload_format_indicator is %d", event->property->payload_format_indicator);
        ESP_LOGI(TAG, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
        ESP_LOGI(TAG, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
        ESP_LOGI(TAG, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        ESP_LOGI(TAG, "msg_id=%d", event->msg_id);
        ESP_LOGI(TAG, "---------------------");

        if(strncmp(event->topic, &mqtt_user_light_state_topic[0], strnlen(&mqtt_user_light_state_topic[0], 128)) == 0) {
            cJSON *root = cJSON_Parse(event->data);
            if (root == NULL) {
                ESP_LOGE(TAG, "Failed to parse JSON data");
            } else {
                cJSON *lightState = cJSON_GetObjectItem(root, &mqtt_user_light_state_json[0]);
                if (lightState != NULL) {
                    if (cJSON_IsTrue(lightState)) {
                        ESP_LOGI(TAG, "Light state is ON");
                        // Here you can add code to turn on the light
                        set_led_state(true);

                    } else if (cJSON_IsFalse(lightState)) {
                        ESP_LOGI(TAG, "Light state is OFF");
                        // Here you can add code to turn off the light
                        set_led_state(false);

                    } else {
                        ESP_LOGE(TAG, "Invalid light state value");
                    }
                } else {
                    ESP_LOGE(TAG, "No lightState field found in JSON data");
                }
                cJSON_Delete(root);
            }
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        print_user_property(event->property->user_property);
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void send_device_presence_update(void){
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object root");
        return;
    }

    cJSON *devices = cJSON_CreateArray();
    if (devices == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON array devices");
        cJSON_Delete(root);
        return;
    }

    for (int i = 0; i < device_list_size; i++) {
        cJSON *device = cJSON_CreateObject();
        if (device == NULL) {
            ESP_LOGE(TAG, "Failed to create JSON object for device %d", i);
            continue;
        }
        cJSON_AddStringToObject(device, "deviceName", &device_list[i].name[0]);
        cJSON_AddStringToObject(device, "deviceId", &device_list[i].id[0]);
        cJSON_AddNumberToObject(device, "distance", device_list[i].distance);
        cJSON_AddBoolToObject(device, "present", device_list[i].present);
        cJSON_AddItemToArray(devices, device);

        device_list[i].present = false; // Reset presence state after sending
    }

    cJSON_AddItemToObject(root, "devices", devices);
    char *json_data = cJSON_Print(root);
    if (json_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON data");
        cJSON_Delete(root);
        return;
    }
    ESP_LOGI(TAG, "Sending device presence update: %s", json_data);
    esp_mqtt5_client_set_user_property(&publish_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_publish_property(globalClient, &publish_property);
    
    esp_mqtt_client_publish(globalClient, &mqtt_user_items_update_topic[0], json_data, 0, 1, 0);

    esp_mqtt5_client_delete_user_property(publish_property.user_property);
    publish_property.user_property = NULL;

    // free the JSON string and cJSON object
    cJSON_free(json_data);
    cJSON_Delete(root);

}

static void check_proxy_done(void *arguments){
    configASSERT( ( ( uint32_t ) arguments ) == 1 );
    
    while(!proxy_connected) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Waiting for proxy connection to be established...");
    }
    ESP_LOGI(TAG, "Proxy connection established, starting MQTT client...");
    esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_disconnect_property(globalClient, &disconnect_property);
    ESP_ERROR_CHECK(esp_mqtt_client_disconnect(globalClient)); // Prvo ugasimo client koji je pokrenut za proxy credentials
    esp_mqtt5_client_delete_user_property(disconnect_property.user_property);

    ESP_ERROR_CHECK(esp_mqtt_client_stop(globalClient)); // Nakon proxy credentials, ugasit proxy client i pokrenit client za slanje podataka
    ESP_ERROR_CHECK(esp_mqtt_client_destroy(globalClient));

    ESP_LOGI(TAG, "MQTT proxy client stopped and destroyed");
    ESP_LOGI(TAG, "Starting MQTT client with user credentials...");
    
    globalClient = esp_mqtt_client_init(&mqtt5_user_cfg);

    assamble_mqtt_user_topics(&mqtt_user_username[0]);

    /* Set connection properties and user properties */
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 500,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = "v1/devices/esp/response",
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_connect_property(globalClient, &connect_property);

    /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
     * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
     */
    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(globalClient, ESP_EVENT_ANY_ID, mqtt5_user_event_handler, NULL);
    esp_mqtt_client_start(globalClient);

    ESP_LOGI(TAG, "MQTT client started with user credentials");
    while(1){
        vTaskDelay(pdMS_TO_TICKS(10000));
        send_device_presence_update();
        ESP_LOGI(TAG, "Device presence update sent");
    }
}


static void mqtt5_app_start(void)
{
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 500,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = "v1/devices/esp/response",
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

    esp_mqtt_client_config_t mqtt5_proxy_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URL,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.reconnect_timeout_ms = 30000,
        .credentials.username = CONFIG_MQTT_PROXY_USERNAME,
        .credentials.authentication.password = CONFIG_MQTT_PROXY_PASSWORD,
        .credentials.client_id = CONFIG_MQTT_PROXY_ID
    };

    // globalClient = esp_mqtt_client_init(&mqtt5_cfg);
    globalClient = esp_mqtt_client_init(&mqtt5_proxy_cfg);
    
    /* Set connection properties and user properties */
    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_connect_property(globalClient, &connect_property);

    /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
     * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
     */
    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(globalClient, ESP_EVENT_ANY_ID, mqtt5_proxy_event_handler, NULL);
    esp_mqtt_client_start(globalClient);
}

void app_main(void)
{
    configure_led();

    // BLUETOOTH INITIALIZATION
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    ble_ibeacon_init();

    // WIFI INITIALIZATION
    ESP_LOGI(TAG, "Start wifi connect.");
    example_wifi_start();

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .scan_method = EXAMPLE_WIFI_SCAN_METHOD,
            .sort_method = EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
            .threshold.authmode = EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
    ESP_ERROR_CHECK(example_wifi_sta_do_connect(wifi_config, true));
    
    /* START iBeacon SCAN */
    esp_ble_gap_set_scan_params(&ble_scan_params);

    /* START MQTT App*/
    mqtt5_app_start();

    BaseType_t xProxyCheckReturned;
    BaseType_t xHCSR04Returned;
    TaskHandle_t xProxyCheckHandle = NULL;
    TaskHandle_t xHCSR04Handle = NULL;

    xHCSR04Returned = xTaskCreate(
        hcsr04_main_loop,       // Function that implements the task
        "hcsr04_main_loop",   
        2048,                  // Stack size in words, not bytes
        (void *)1,            // Parameter passed into the task
        tskIDLE_PRIORITY,
        &xHCSR04Handle               // Pointer to the task handle
    );

    /* Create the task, storing the handle. */
    xProxyCheckReturned = xTaskCreate(
        check_proxy_done,       // Function that implements the task
        "check_proxy_done",   
        4096,                  // Stack size in words, not bytes
        (void *)1,            // Parameter passed into the task
        tskIDLE_PRIORITY,
        &xProxyCheckHandle               // Pointer to the task handle
    );

}   
