/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/usb_serial_jtag.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_check.h"

#if 1
#include "hal/usb_serial_jtag_ll.h"
#include "driver/usb_serial_jtag.h"
#endif

#define BUF_SIZE (1024)
#define ECHO_TASK_STACK_SIZE (4096)

static uint8_t *data = 0;
static void echo_task(void *arg)
{
    // Configure USB SERIAL JTAG
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));
    ESP_LOGI("usb_serial_jtag echo", "USB_SERIAL_JTAG init done");

    // Configure a temporary buffer for the incoming data
    data = (uint8_t *) malloc(BUF_SIZE);
    if (data == NULL) {
        ESP_LOGE("usb_serial_jtag echo", "no memory for data");
        return;
    }
	data[0] = '\0';
	
	vTaskDelay(pdMS_TO_TICKS(2000));
	

	const char wrapline[] = "\n";
	const char prompt[] = "> ";
	char line[BUF_SIZE] = {'\0'};
    while (1) {
        int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);

        // Write data back to the USB SERIAL JTAG
        if (len) {
            usb_serial_jtag_write_bytes((const char *) data, len, 20 / portTICK_PERIOD_MS);
            data[len] = '\0';
            //ESP_LOG_BUFFER_HEXDUMP("Recv str: ", data, len, ESP_LOG_INFO);
#if 0			
			strcat(line, (const char *)data);
			if (data[0] == '\r' || data[0] == '\n') {
				usb_serial_jtag_write_bytes(wrapline, sizeof(wrapline), 20 / portTICK_PERIOD_MS);
				//usb_serial_jtag_write_bytes(line, strlen(line), 20 / portTICK_PERIOD_MS);
				//tinybasic_parse(line);
				line[0] = '\0';
				usb_serial_jtag_write_bytes(prompt, sizeof(prompt), 20 / portTICK_PERIOD_MS);
			}
#endif
        }
    }
}

extern int main__(int argc, char *argv[]);
static void arduino_task(void *arg)
{
	char argv0[] = "tiny386";
	char argv1[] = "tiny386.ini";
	char *argv[] = {argv0, argv1};
	main__(2, argv);
}

void app_main(void)
{
    xTaskCreate(echo_task, "USB SERIAL JTAG_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(arduino_task, "arduino_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}



void writeUartConsole(uint32_t val) {
	char data[2] = {0};
	data[0] = (char)val;
	usb_serial_jtag_write_bytes((const char *) data, 1, 1 / portTICK_PERIOD_MS);
	//usb_serial_jtag_write_bytes("\b", 1, 2 / portTICK_PERIOD_MS);
	usb_serial_jtag_ll_txfifo_flush();
}

int _ReadKBByte(void)
{
	if (data && data[0]) { //strlen((const char *)data) > 0) {
		uint8_t result = data[0];
		data[0] = '\0';
		return result;
	}
	return -1;
}

int _IsKBHit(void)
{
    if (data && data[0]) { //strlen((const char *)data) > 0) {
		return 1;
	}
	return 0;
}
