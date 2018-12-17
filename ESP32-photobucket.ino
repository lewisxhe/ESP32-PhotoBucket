#include <WiFi.h>
#include <Esp.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "GfxUi.h"
#include "photobucket.h"
#include "config.h" //The first time you need to edit the config file


// Bitmap_WiFi
extern uint8_t wifi_1[];
extern uint8_t wifi_2[];
extern uint8_t wifi_3[];

SPIClass SDSPI(HSPI);
TFT_eSPI tft = TFT_eSPI();
GfxUi ui = GfxUi(&tft);
PHOTOBUCCKET photoWeb(USER_NAME);


int files;
int searchPhotoFiles(fs::FS &fs, const char *dirname, uint8_t levels, void (*callback)(const char *filename, int type));
void darwPhotoTask(const char *filename, int type);

void downloadCallback(String filename, uint32_t bytesDownloaded, uint32_t bytesTotal)
{
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setTextPadding(320);
    int percentage = 100 * bytesDownloaded / bytesTotal;
    if (percentage == 0)
    {
        tft.drawString(filename, 120, 220);
    }
    if (percentage % 5 == 0)
    {
        tft.setTextDatum(TC_DATUM);
        tft.setTextPadding(tft.textWidth(" 888% "));
        tft.drawString(String(percentage) + "%", 120, 245);
        ui.drawProgressBar(10, 225, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);
    }
}

void downloadTask(void *param)
{
    // photoWeb.setProgressCallback(downloadCallback);
    photoWeb.testGET();
    photoWeb.downloadPhoto();
    vTaskDelete(NULL);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }
    Serial.printf("SPRAM:%lu\n", ESP.getPsramSize());

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);

    SDSPI.begin(SPI_SD_CLK, SPI_SD_MISO, SPI_SD_MOSI, SPI_SD_CS);
    if (!FILESYSTEM.begin(SPI_SD_CS, SDSPI))
    {
        tft.println("FILESYSTEM Mount FAIL");
        while (1)
        {
            delay(1000);
        }
    }
    uint32_t cardSize = FILESYSTEM.cardSize() / 1024 / 1024;
    uint32_t totalBytes = FILESYSTEM.totalBytes() / 1024;
    uint32_t usedBytes = FILESYSTEM.usedBytes() / 1024;

    Serial.printf("cardSize:%lu totalBytes:%lu usedBytes:%lu\n", cardSize, totalBytes, usedBytes);
    tft.println("SD Card Size:" + String(cardSize) + "MB");
    delay(200);
    tft.println("WiFi Init Begin ...");
    delay(200);

    tft.setCursor(0, 170);
    tft.setTextSize(2);

    tft.print("Connecting to:");
    tft.println(NETWORK_SSID);

    int i = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(200);
        tft.drawBitmap(70, 50, wifi_1, 100, 100, TFT_WHITE);
        delay(200);
        tft.drawBitmap(70, 50, wifi_2, 100, 100, TFT_WHITE);
        delay(200);
        tft.drawBitmap(70, 50, wifi_3, 100, 100, TFT_WHITE);
        delay(200);
        tft.fillRect(70, 50, 100, 100, TFT_BLACK);
        if (++i > 20)
        {
            esp_restart();
        }
    }
    tft.print("WiFi connected:");
    tft.println(WiFi.localIP());
    delay(1500);


    files = searchPhotoFiles(FILESYSTEM, "/", 2, NULL);
    Serial.printf("Search Photo : %d\n", files);

    tft.fillScreen(TFT_BLACK);
    xTaskCreate(downloadTask, "", 4096, NULL, 2, NULL);
}

int searchPhotoFiles(fs::FS &fs, const char *dirname, uint8_t levels, void (*callback)(const char *filename, int type))
{
    int photoFiles = 0;
    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return 0;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return 0;
    }
    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            // Serial.print("  FILE: ");
            // Serial.print(file.name());
            // Serial.print("\tSIZE: ");
            // Serial.println(file.size());
            String name = String(file.name());
            if (name.endsWith(".jpg") || name.endsWith(".JPG") || name.endsWith(".jpeg"))
            {
                photoFiles++;
                if (callback)
                    callback(file.name(), 0);
            }
            else if (name.endsWith(".bmp") || name.endsWith(".BMP"))
            {
                photoFiles++;
                if (callback)
                    callback(file.name(), 1);
            }
        }
        file = root.openNextFile();
    }
    return photoFiles;
}

void darwPhotoTask(const char *filename, int type)
{
    switch (type)
    {
    case 0:
        ui.drawJpeg(filename, 0, 0);
        break;
    case 1:
        ui.drawBmp(filename, 0, 0);
        break;
    default:
        break;
    }
    delay(5000);
}

void loop()
{
    if (files)
        searchPhotoFiles(FILESYSTEM, "/", 2, darwPhotoTask);
}