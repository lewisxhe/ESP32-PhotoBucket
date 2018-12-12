#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Esp.h>
#include <TFT_eSPI.h>
#include "photobucket.h"
#include <SPI.h>
#include "config.h"
#include <JPEGDecoder.h>
#include "GfxUi.h"
// #include "miniz.h"
// #include "mini_gzip.h"

#if USE_SD
SPIClass SDSPI(HSPI);
#endif

TFT_eSPI tft = TFT_eSPI();
GfxUi ui = GfxUi(&tft);
PHOTOBUCCKET photoWeb;
// AsyncWebServer server(80);

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_RED);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);

#if USE_SD
    SDSPI.begin(SPI_SD_CLK, SPI_SD_MISO, SPI_SD_MOSI, SPI_SD_CS);
    if (!FILESYSTEM.begin(SPI_SD_CS, SDSPI))
#elif USE_SPIFFS
    if (!FILESYSTEM.begin())
#endif
    {
        Serial.println("FILESYSTEM is not database");
        Serial.println("Please use Arduino ESP32 Sketch data Upload files");
        while (1)
        {
            delay(1000);
        }
    }
    Serial.print(" SPRAM:");
    Serial.println(ESP.getPsramSize());

    WiFi.mode(WIFI_STA);
    WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        esp_restart();
    }

    Serial.println(F("WiFi connected"));
    Serial.println("");
    Serial.println(WiFi.localIP());

    // if (photoWeb.login(USER_NAME, PASSWORD))
    // photoWeb.downloadPhoto();

    // server.on("/jpg", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     request->send(FILESYSTEM, "/1540550352308_zpsexn6v5oj.jpg", "image/jpg");
    // });

    // server.onNotFound([](AsyncWebServerRequest *request) {
    //     request->send(404, "text/plain", "Not found");
    // });

    // server.begin();
}

void searchJPEG(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
            String name = String(file.name());
            if (name.endsWith(".jpg"))
            {
                tft.fillScreen(random(0xFFFF));
                ui.drawJpeg(file.name(), 0, 0);
                delay(5000);
            }
        }
        file = root.openNextFile();
    }
}

void loop()
{
    searchJPEG(FILESYSTEM, "/", 2);
}