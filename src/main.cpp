// demonstrates the speed improvements when enabling FF_USE_FASTSEEK
#include <errno.h>
#include "Arduino.h"
#include "SPI.h"
#include "SD.h"
#include "esp_system.h"

// SD card wiring
#define GPIO_SD_MISO GPIO_NUM_35
#define GPIO_SD_CS   GPIO_NUM_14
#define GPIO_SD_MOSI GPIO_NUM_32
#define GPIO_SD_CLK  GPIO_NUM_33

// will be created / overwritten
#define TEST_FILE "/sd/test.dat"
#define TEST_FILE_SIZE 200 * 1024 * 1024  // [bytes]

void setup()
{
	FILE *f = NULL;

	// Mount SD card with non standard pinout
	SPI.begin(GPIO_SD_CLK, GPIO_SD_MISO, GPIO_SD_MOSI);
	SD.begin(GPIO_SD_CS, SPI, 20 * 1000 * 1000, "/sd", 3);

	// Create an empty file of TEST_FILE_SIZE
	f = fopen(TEST_FILE, "wb");
	if (!f) {
		log_e("fopen(%s, wb) failed: %s", TEST_FILE, strerror(errno));
		return;
	}

	log_i("Creating %d MB file: %s ...", TEST_FILE_SIZE / 1024 / 1024, TEST_FILE);
	fseek(f, TEST_FILE_SIZE, SEEK_SET);
	if (fputc('\0', f) != '\0') {
		log_e("fputc() failed");
		fclose(f);
		return;
	}
	fclose(f);

	// Open and seek around in TEST_FILE
	f = fopen(TEST_FILE, "rb");
	if (!f) {
		log_e("fopen(%s, rb) failed: %s", TEST_FILE, strerror(errno));
		return;
	}

	char dummy[4096];  // For dummy reads
	unsigned dest = TEST_FILE_SIZE / 2 + 150;

	while(true) {
		int64_t ts_start = esp_timer_get_time();
		fseek(f, dest, SEEK_SET);
		unsigned dt = esp_timer_get_time() - ts_start;

		unsigned ret = fread(dummy, 1, sizeof(dummy), f);

		log_i("fseek(%10d) dt: %6d us, read %4d", dest, dt, ret);

		dest = (dest + 1) % (TEST_FILE_SIZE);
		// dest = esp_random() % TEST_FILE_SIZE;
	}

}

void loop() {
	vTaskDelete(NULL);
}

// ------------------------
//  without FAST_SEEK
// ------------------------
// [I][main.cpp:34] setup(): Creating 200 MB file: /sd/test.dat ...
// [I][main.cpp:58] setup(): fseek( 104857750) dt: 128002 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857751) dt: 128022 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857752) dt: 128044 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857753) dt: 128061 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857754) dt: 128012 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857755) dt: 127987 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857756) dt: 128001 us, read 4096

// ------------------------
//  with FAST_SEEK enabled
// ------------------------
// (comment last line in platform.ini)
// [I][main.cpp:34] setup(): Creating 200 MB file: /sd/test.dat ...
// [V][ff.c:3736] f_open(): FAST_SEEK enabled
// [I][main.cpp:58] setup(): fseek( 104857750) dt:   1143 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857751) dt:    692 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857752) dt:    684 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857753) dt:    681 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857754) dt:    679 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857755) dt:    682 us, read 4096
// [I][main.cpp:58] setup(): fseek( 104857756) dt:    681 us, read 4096
