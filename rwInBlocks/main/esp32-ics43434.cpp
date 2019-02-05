//inspired by: https://forums.adafruit.com/viewtopic.php?f=57&t=124924&p=625289&hilit=I2S#p625289 #include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "driver/sdmmc_defs.h"

#define BUFFER_LEN 512

namespace {
	static const char *TAG = "example";
	sdmmc_host_t host;
	char text[BUFFER_LEN] = "aloha from the SD card";
	sdmmc_slot_config_t slot_config;
	sdmmc_card_t card;
};


void rwBlocksDemo()
{
	esp_err_t ret;
	
	//disk stuff
	ESP_LOGI(TAG, "Using SDMMC peripheral");
	host = SDMMC_HOST_DEFAULT();
	host.flags = SDMMC_HOST_FLAG_4BIT;
	host.max_freq_khz = SDMMC_FREQ_DEFAULT;
	// This initializes the slot without card detect (CD) and write protect (WP) signals.
	// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
	slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	
	
	sdmmc_host_init();
	ret = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1,&slot_config);
	for (;;){
		ret = sdmmc_card_init(&host, &card);
		if(ret ==ESP_OK) break;
		ESP_LOGW(TAG, "slave init failed, retrying");
		vTaskDelay(100);
	}

	sdmmc_card_print_info(stdout, &card);
	ESP_LOGI(TAG, "block_size is %d", card.csd.sector_size);
	ESP_LOGI(TAG, "block_capacity is %d", card.csd.capacity);

	// GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
	// Internal pull-ups are not sufficient. However, enabling internal pull-ups
	// does make a difference some boards, so we do that here.
	gpio_set_pull_mode(((gpio_num_t)15), GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
	gpio_set_pull_mode(((gpio_num_t)2), GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
	gpio_set_pull_mode(((gpio_num_t)4), GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
	gpio_set_pull_mode(((gpio_num_t)12), GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
	gpio_set_pull_mode(((gpio_num_t)13), GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes


	if (ret == ESP_OK){
		ESP_LOGI(TAG, "Card Initted");
		ret = sdmmc_write_sectors(&card, text, 0x1FF, 1);
		if (ret == ESP_OK){
			ESP_LOGI(TAG, "wrote blocks");
			vTaskDelay(400);
			memset(text, 0x0, BUFFER_LEN);
			ret = sdmmc_read_sectors(&card, text, 0x1FF, 1);
			if(ret == ESP_OK)
			{ 
				ESP_LOGI(TAG, "read blocks: %s", text);
			} else ESP_LOGE(TAG, "couldn't read");
		}else ESP_LOGE(TAG, "couldn't write");
	}else ESP_LOGE(TAG, "couldn't init");
}

extern "C" void app_main()
{
	rwBlocksDemo();
}
