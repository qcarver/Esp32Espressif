//inspired by: https://forums.adafruit.com/viewtopic.php?f=57&t=124924&p=625289&hilit=I2S#p625289

#include <string.h>
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
#include "driver/i2s.h"
#include "esp_system.h"
#include <stdio.h>
#include <stdint.h>

#define AUDIO_RATE           44100
#define PIN_NUM_MISO ((gpio_num_t)VSPI_IOMUX_PIN_NUM_MISO)
#define PIN_NUM_MOSI ((gpio_num_t)VSPI_IOMUX_PIN_NUM_MOSI)
#define PIN_NUM_CLK  ((gpio_num_t)VSPI_IOMUX_PIN_NUM_CLK)
#define PIN_NUM_CS   ((gpio_num_t)VSPI_IOMUX_PIN_NUM_CS)

namespace {

const int sample_rate = 44100;

/* RX: I2S_NUM_1 */
i2s_config_t i2s_config_rx  = {
  mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  sample_rate: sample_rate,
  bits_per_sample: I2S_BITS_PER_SAMPLE_32BIT, //(i2s_bits_per_sample_t)48,    // Only 8-bit DAC support
  channel_format: I2S_CHANNEL_FMT_ONLY_RIGHT,   // 2-channels
  communication_format: (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB),
  intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,        // Interrupt level 1
  dma_buf_count: 8,                            // number of buffers, 128 max.
  dma_buf_len: 8                          // size of each buffer
};

i2s_pin_config_t pin_config_rx = {
  bck_io_num: GPIO_NUM_26,
  ws_io_num: GPIO_NUM_25,
  data_out_num: I2S_PIN_NO_CHANGE,
  data_in_num: GPIO_NUM_22
};
  FILE* f = nullptr;	
  static const char *TAG = "example";
  uint32_t num_samples = 0;
};


void setup()
{
  //mic stuff
  ESP_LOGI(TAG, "Using I2S peripheral");
  i2s_driver_install(I2S_NUM_1, &i2s_config_rx, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin_config_rx);
  i2s_zero_dma_buffer(I2S_NUM_1);
  //disk stuff
  ESP_LOGI(TAG, "Initializing SD card");

  ESP_LOGI(TAG, "Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // To use 1-line SD mode, uncomment the following line:
  // host.flags = SDMMC_HOST_FLAG_1BIT;

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
  // Internal pull-ups are not sufficient. However, enabling internal pull-ups
  // does make a difference some boards, so we do that here.
  gpio_set_pull_mode(((gpio_num_t)15), GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
  gpio_set_pull_mode(((gpio_num_t)2), GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
  gpio_set_pull_mode(((gpio_num_t)4), GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
  gpio_set_pull_mode(((gpio_num_t)12), GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
  gpio_set_pull_mode(((gpio_num_t)13), GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes


  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
  // Please check its source code and implement error recovery when developing
  // production applications.
  sdmmc_card_t* card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE(TAG, "Failed to mount filesystem. "
              "If you want the card to be formatted, set format_if_mount_failed = true.");
      } else {
          ESP_LOGE(TAG, "Failed to initialize the card (%s). "
              "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
      }
      return;
  }

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);

  // Use POSIX and C standard library functions to work with files.
  // First create a file.

 f = fopen("/sdcard/sound.raw", "w");
 if (f == NULL) {
     ESP_LOGE(TAG, "Failed to open file for writing");
     return;
 }
  
}

void printBarForGraph(const int32_t value) {
  const uint8_t NUMCHARS = 16;
  const unsigned int STEP_SIZE = 0xFFFFFF / NUMCHARS;
  const unsigned int SUB_STEP_SIZE = STEP_SIZE/8;
  const uint8_t FULL_BLOCKS = value / STEP_SIZE;
  const uint8_t SUB_BLOCKS = (value - ((value / STEP_SIZE) * STEP_SIZE)/SUB_STEP_SIZE);

  for (int fullBlocks = 0; fullBlocks <= FULL_BLOCKS; fullBlocks++) {
    printf("█");
  }

  switch (SUB_BLOCKS) {
    case (0): break;
    case (1): printf("▏"); break;
    case (2): printf("▎"); break;
    case (3): printf("▍"); break;
    case (4): printf("▌"); break;
    case (5): printf("▋"); break;
    case (6): printf("▊"); break;
    case (7): printf("▉"); break;
  }
  printf(" %d\n", num_samples);
}

void loop() {
  int32_t mic_sample = 0;

  //read 24 bits of signed data into a 48 bit signed container
  if (i2s_pop_sample(I2S_NUM_1, (char*)&mic_sample, portMAX_DELAY) == 4) {

    //like: https://forums.adafruit.com/viewtopic.php?f=19&t=125101 ICS-43434 is 1 pulse late
    //The MSBit is actually a false bit (always 1), the MSB of the real data is actually the second bit

    //Porting a 23 signed number into a 32 bits we note sign which is '-' if bit 23 is '1'
    mic_sample = (mic_sample & 0x400000) ?
                 //Negative: B/c of 2compliment unused bits left of the sign bit need to be '1's
                 (mic_sample | 0xFF800000) :
                 //Positive: B/c of 2compliment unused bits left of the sign bit need to be '0's
                 (mic_sample & 0x7FFFFF);

    //B/c the MSB was late, the LSBit is past the sample window, for us LSBit is always zero
    mic_sample <<= 1; //scale back up to proper 3 byte size unless you don't care

    printBarForGraph(abs(mic_sample));
    //printf("\t\t\t\t%d", mic_sample);
    fwrite(&mic_sample,sizeof(uint8_t),3,f);
    num_samples++;	
  }
}

extern "C" void app_main()
{
	setup();
	while(1){
		loop();
		if (num_samples == (sample_rate * 1))
                {
		  ESP_LOGI(TAG, "*********************************************************File written");
                  fclose(f);
		  esp_vfs_fat_sdmmc_unmount();
		  for (;1;)
		  ESP_LOGI(TAG, "Card unmounted");
		}
	}
}
