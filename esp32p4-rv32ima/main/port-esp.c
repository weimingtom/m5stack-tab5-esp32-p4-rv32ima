/*
 * Copyright (c) 2023, Jisheng Zhang <jszhang@kernel.org>. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_flash.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_partition.h"
#include "hal/usb_serial_jtag_ll.h"
#include "psram.h"
#include "driver/uart.h" //added, for uart_read_bytes() and uart_get_buffered_data_len()

//see also https://github.com/hchunhui/tiny386/blob/master/misc.c
//see CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#define USE_ESP_UART 1

#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#define BUF_SIZE (1024)
uint8_t *data;
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
#if 0
    while (1) {

        int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);

        // Write data back to the USB SERIAL JTAG
        if (len) {
            usb_serial_jtag_write_bytes((const char *) data, len, 20 / portTICK_PERIOD_MS);
            data[len] = '\0';
            ESP_LOG_BUFFER_HEXDUMP("Recv str: ", data, len, ESP_LOG_INFO);
        }
    }
#endif	
}


static void configure_uarts() {
#if 0	
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity	= UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};

	uart_param_config(UART_NUM_0, &uart_config);
	if (uart_driver_install(UART_NUM_0, 2 * 1024, 0, 0, NULL, 0) != ESP_OK) {
		assert(false);
	}
#else
	echo_task(NULL);
#endif	
}

uint64_t GetTimeMicroseconds()
{
	return esp_timer_get_time();
}

int ReadKBByte(void)
{
#if !USE_ESP_UART	
	uint8_t rxchar;
	int rread;

	rread = usb_serial_jtag_ll_read_rxfifo(&rxchar, 1);

	if (rread > 0)
		return rxchar;
	else
		return -1;
#else

	
/*
	char data;
	if (uart_read_bytes(0, &data, 1, 20 / portTICK_PERIOD_MS) > 0) {
		return data;
	}
	return -1;
*/
	if (data && strlen((const char *)data) > 0) {
		uint8_t result = data[0];
		data[0] = '\0';
		return result;
	}
	return -1;
#endif	
}

int IsKBHit(void)
{
#if !USE_ESP_UART
	return usb_serial_jtag_ll_rxfifo_data_available();
#else
/*
	size_t len;
	if (uart_get_buffered_data_len(0, &len) == ESP_OK) {
		if (len)
			return 1;
	}
	return 0;
*/
    if (data && strlen((const char *)data) > 0) {
		return 1;
	}
	//FIXME: if 2 / portTICK_PERIOD_MS is too large, it will be very slow 
	int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE - 1), 2 / portTICK_PERIOD_MS);
	if (len) {
		//usb_serial_jtag_write_bytes((const char *) data, len, 2 / portTICK_PERIOD_MS);
        //ESP_LOGE("usb_serial_jtag_read_bytes", "%02X", (uint8_t)data[0]);
		//printf("%c", (uint8_t)data[0]); fflush(stdout);
#if 0
//no need, if use usb_serial_jtag_write_bytes in uc-rv32ima.c:HandleControlStore()
		usb_serial_jtag_write_bytes((const char *) data, 1, 2 / portTICK_PERIOD_MS);
        usb_serial_jtag_write_bytes("\b", 1, 2 / portTICK_PERIOD_MS);
		usb_serial_jtag_ll_txfifo_flush();
#endif
		data[len] = '\0';
		return 1;
	}
	return 0;
#endif
}

static uint8_t *psram_base = NULL;
static size_t psram_size = 0;

int psram_init(void)
{
	size_t available_psram;
	size_t alloc_size;

	available_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

	if (available_psram == 0) {
		printf("ERROR: No PSRAM available!\n");
		printf("Check menuconfig: Component config -> ESP PSRAM\n");
		return -1;
	}

	printf("Available PSRAM: %zu bytes (%.2f MB)\n",
		   available_psram, available_psram / (1024.0 * 1024.0));

	alloc_size = (available_psram * 9) / 10;

	printf("Attempting to allocate %zu bytes (%.2f MB)...\n",
		   alloc_size, alloc_size / (1024.0 * 1024.0));

	psram_base = (uint8_t *)heap_caps_malloc(alloc_size, MALLOC_CAP_SPIRAM);

	if (psram_base == NULL) {
		printf("ERROR: Failed to allocate PSRAM!\n");
		return -1;
	}

	psram_size = alloc_size;
	printf("SUCCESS: PSRAM allocated at %p, size: %zu bytes (%.2f MB)\n",
		   psram_base, psram_size, psram_size / (1024.0 * 1024.0));

	printf("Initializing PSRAM to zero...\n");
	memset(psram_base, 0, psram_size);
	printf("PSRAM initialized successfully!\n");

#if USE_ESP_UART	
	configure_uarts();
#endif
	
	return 0;
}

int psram_read(uint32_t addr, void *buf, int len)
{
	if (psram_base == NULL) {
		printf("ERROR: psram_read called before psram_init!\n");
		return -1;
	}

	if (addr + len > psram_size) {
		printf("ERROR: psram_read out of bounds: addr=0x%lx, len=%d, size=%zu\n",
			   (unsigned long)addr, len, psram_size);
		return -1;
	}

	memcpy(buf, psram_base + addr, len);
	return len;
}

int psram_write(uint32_t addr, void *buf, int len)
{
	if (psram_base == NULL) {
		printf("ERROR: psram_write called before psram_init!\n");
		return -1;
	}

	if (addr + len > psram_size) {
		printf("ERROR: psram_write out of bounds: addr=0x%lx, len=%d, size=%zu\n",
			   (unsigned long)addr, len, psram_size);
		return -1;
	}

	memcpy(psram_base + addr, buf, len);
	return len;
}

void *psram_get_base(void)
{
	return psram_base;
}

size_t psram_get_size(void)
{
	return psram_size;
}

void verify_kernel_header(void)
{
	uint8_t header[64];

	printf("\n=== Verifying Kernel Header ===\n");
	psram_read(0, header, 64);

	printf("First 64 bytes of loaded kernel:\n");
	for (int i = 0; i < 64; i++) {
		printf("%02x ", header[i]);
		if ((i + 1) % 16 == 0) printf("\n");
	}
	printf("\n");

	// Check RISC-V magic
	if (header[0x30] == 'R' && header[0x31] == 'I' &&
		header[0x32] == 'S' && header[0x33] == 'C' &&
		header[0x34] == 'V') {
		printf("✓ RISCV magic found at offset 0x30\n");
		} else {
			printf("✗ RISCV magic NOT found! Expected at 0x30\n");
		}

		uint32_t first_instr = *(uint32_t*)header;
	printf("First instruction: 0x%08lx\n", (unsigned long)first_instr);
	printf("Expected: 0x05c0006f (j 0x5c)\n\n");
}

int load_images(int ram_size, int *kern_len)
{
	const esp_partition_t *kernel_partition;
	esp_err_t err;
	uint32_t addr;
	char dmabuf[64];
	size_t partition_size;

	printf("\n=== Loading Kernel from Flash ===\n");

	kernel_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
												ESP_PARTITION_SUBTYPE_ANY,
											 "kernel");
	if (kernel_partition == NULL) {
		printf("ERROR: 'kernel' partition not found!\n");
		printf("Make sure partition table has 'kernel' partition\n");
		return -1;
	}

	partition_size = kernel_partition->size;
	printf("Found kernel partition:\n");
	printf("  Label: %s\n", kernel_partition->label);
	printf("  Address: 0x%lx\n", (unsigned long)kernel_partition->address);
	printf("  Size: %zu bytes (%.2f MB)\n", partition_size,
		   partition_size / (1024.0 * 1024.0));

	if (partition_size > ram_size) {
		printf("WARNING: Partition size (%zu) > RAM size (%d)\n",
			   partition_size, ram_size);
		printf("Will only load first %d bytes\n", ram_size);
		partition_size = ram_size;
	}

	if (partition_size > psram_get_size()) {
		printf("WARNING: Partition size (%zu) > PSRAM size (%zu)\n",
			   partition_size, psram_get_size());
		partition_size = psram_get_size();
	}

	if (kern_len)
		*kern_len = partition_size;

	printf("\nLoading kernel from flash to PSRAM...\n");
	printf("This will take a moment...\n");

	addr = 0;
	size_t remaining = partition_size;

	while (remaining >= 64) {
		err = esp_partition_read(kernel_partition, addr, dmabuf, 64);
		if (err != ESP_OK) {
			printf("\nERROR: Failed to read from flash at offset %lu: %s\n",
				   (unsigned long)addr, esp_err_to_name(err));
			return -1;
		}

		psram_write(addr, dmabuf, 64);
		addr += 64;
		remaining -= 64;

		if ((addr % (64 * 1024)) == 0) {
			printf(".");
			fflush(stdout);
		}
	}

	if (remaining > 0) {
		err = esp_partition_read(kernel_partition, addr, dmabuf, remaining);
		if (err != ESP_OK) {
			printf("\nERROR: Failed to read remaining bytes: %s\n",
				   esp_err_to_name(err));
			return -1;
		}
		psram_write(addr, dmabuf, remaining);
	}

	printf("\n✓ Kernel loaded successfully from flash!\n");
	printf("Total loaded: %zu bytes (%.2f MB)\n",
		   partition_size, partition_size / (1024.0 * 1024.0));

	verify_kernel_header();

	return 0;
}
