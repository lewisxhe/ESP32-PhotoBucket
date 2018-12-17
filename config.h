#pragma onec
#include "passwd.h"
#include <FS.h>
#include <SD.h>

#define FILESYSTEM SD
#define SPI_SD_CS 13
#define SPI_SD_MISO 2
#define SPI_SD_MOSI 15
#define SPI_SD_CLK 14

#define BUTTON_1 37
#define BUTTON_2 38
#define BUTTON_3 39

#ifndef NETWORK_SSID
#define NETWORK_SSID "<WIFI SSID>"
#define NETWORK_PASSWORD "<WIFI PASSWORD>"
// http://s1268.photobucket.com/   
#define USER_NAME "<YOUR USERNAME>"     //  Your photobucket username 
#endif