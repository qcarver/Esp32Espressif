//inspired by: https://forums.adafruit.com/viewtopic.php?f=57&t=124924&p=625289&hilit=I2S#p625289

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_system.h"
#include <stdio.h>
#include <stdint.h>


#define AUDIO_RATE           44100

namespace {

const int sample_rate = 44100;

/* RX: I2S_NUM_1 */
i2s_config_t i2s_config_rx  = {
mode: (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
sample_rate: sample_rate,
bits_per_sample: I2S_BITS_PER_SAMPLE_24BIT, //(i2s_bits_per_sample_t)48,    // Only 8-bit DAC support
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
};


void setup()
{
  i2s_driver_install(I2S_NUM_1, &i2s_config_rx, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin_config_rx);
  i2s_zero_dma_buffer(I2S_NUM_1);
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
  printf("\n");
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
  }
}

extern "C" void app_main()
{
	setup();
	while(1){
		loop();
	}
}
