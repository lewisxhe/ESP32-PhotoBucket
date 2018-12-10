#pragma onec

#define USE_SD 1
#define USE_SPIFFS 0

#include "passwd.h"
#include <FS.h>

#if USE_SD
#include <SD.h>
#define FILESYSTEM SD
#define SPI_SD_CS 13
#define SPI_SD_MISO 2
#define SPI_SD_MOSI 15
#define SPI_SD_CLK 14
#elif USE_SPIFFS
#include <SPIFFS.h>
#define FILESYSTEM SPIFFS
#else
#error "Please select Filesysytem type"
#endif



#ifndef NETWORK_SSID
#define NETWORK_SSID ""
#define NETWORK_PASSWORD ""
#define USER_NAME ""
#define PASSWORD ""
#endif