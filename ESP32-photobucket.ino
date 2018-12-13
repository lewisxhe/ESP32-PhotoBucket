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
#include <OneButton.h>

// Bitmap_WiFi
extern uint8_t wifi_1[];
extern uint8_t wifi_2[];
extern uint8_t wifi_3[];

#if USE_SD
SPIClass SDSPI(HSPI);
#endif

TFT_eSPI tft = TFT_eSPI();
GfxUi ui = GfxUi(&tft);
PHOTOBUCCKET photoWeb(USER_NAME);
// AsyncWebServer server(80);

#define BUTTON_1 37
#define BUTTON_2 38
#define BUTTON_3 39

OneButton button1(BUTTON_1, true);
OneButton button2(BUTTON_2, true);
OneButton button3(BUTTON_3, true);
int files;
int searchPhotoFiles(fs::FS &fs, const char *dirname, uint8_t levels, void (*callback)(const char *filename, int type));
void darwPhotoTask(const char *filename, int type);

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
            Serial.print(" REMOVE FILE: ");
            Serial.print(file.name());
            FILESYSTEM.remove("/" + String(file.name()));
        }
        file = root.openNextFile();
    }
}

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

void click1()
{
    Serial.println(__func__);
}
void click2()
{
    Serial.println(__func__);
}
void click3()
{
    Serial.println(__func__);
}
void buttonTask(void *param)
{
    for (;;)
    {
        button1.tick();
        button2.tick();
        button3.tick();
        delay(5);
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
    tft.fillScreen(TFT_BLACK);
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
        tft.println("FILESYSTEM Mount FAIL");
        while (1)
        {
            delay(1000);
        }
    }
    uint32_t sdSize = FILESYSTEM.cardSize() / 1024 / 1024;
    tft.println("SD Card Size:" + String(sdSize) + "MB");
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

    button1.attachClick(click1);
    button2.attachClick(click2);
    button3.attachClick(click3);

    xTaskCreate(buttonTask, "", 2048, NULL, 2, NULL);

    photoWeb.testGET();
    while(1);
    // if (photoWeb.login(USER_NAME, PASSWORD))
    // {
    //     tft.fillScreen(TFT_BLACK);
    //     photoWeb.downloadPhoto(downloadCallback);
    // }

    if ((files = searchPhotoFiles(FILESYSTEM, "/", 2, NULL)) == 0)
    {
        tft.setTextDatum(BC_DATUM);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.fillScreen(TFT_BLACK);
        tft.drawString("No search photo files", 120, 240);
        while (1)
        {
            delay(100000);
        }
    }
    Serial.printf("Search Photo : %d\n", files);
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
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());

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
    searchPhotoFiles(FILESYSTEM, "/", 2, darwPhotoTask);
}