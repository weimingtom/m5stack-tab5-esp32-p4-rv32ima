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

static void arduino_task(void *arg)
{
	extern void setup();
	extern void loop();
	setup();
	while (1) {
		loop();
	}
}

void app_main(void)
{
    xTaskCreate(echo_task, "USB SERIAL JTAG_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(arduino_task, "arduino_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}


#define OUTPUT 1
extern void pinMode(int a, int b) {
	
}

extern void Serial_begin(long n) {
	
}

extern void digitalWrite(int port, int value) {
	
}

extern int Serial_read() {
	int res = data[0];
	data[0] = '\0';
	return res;
}

extern int Serial_available() {
	return data && data[0];
}

extern uint8_t pgm_read_byte_near(const uint8_t *ptr) {
	return *ptr;
}

extern void Serial_print(const char *str) {
	usb_serial_jtag_write_bytes(str, strlen(str), 20 / portTICK_PERIOD_MS);
}

static char str[256] = {0};
extern void Serial_print_int(int n) {
	sprintf(str, "%d", n);
	Serial_print(str);
}

extern void Serial_print_char(char ch) {
	sprintf(str, "%c", ch);
	Serial_print(str);
}
