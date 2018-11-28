
#include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include "httpsConverter.h"
// #include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include <Esp.h>
#include "photobucket.h"
#include "passwd.h"
#include <SPIFFS.h>
#include <FS.h>
#define FILESYSTEM SPIFFS

#include "miniz.h"
#include "mini_gzip.h"

PHOTOBUCCKET photoWeb;
// AsyncWebServer server(80);
const char *ssid = NETWORK_SSID;      // Put your SSID here
const char *password = NETWORK_PASSWORD; // Put your PASSWORD here

String str = "HTTP/1.1 200 OK\r\nServer: WebServer\r\n"
             "Date: Tue, 20 Nov 2018 11:05:27 GMT\r\n"
             "Content-Type: application/octet-stream\r\n"
             "Content-Length: 29738\r\n"
             "Connection: keep-alive\r\n"
             "pragma: public\r\n"
             "expires: -1\r\n"
             "cache-control: public, must-revalidate, post-check=0, pre-check=0\r\n"
             "content-transfer-encoding: binary\r\n"
             "content-description: file transfer\r\n"
             "content-disposition: attachment; filename=\"TIM.bmp\"\r\n"
             "Access-Control-Allow-Origin: https://www.onlineconverter.com\r\n"
             "Vary: Origin\r\n"
             "Content-Security-Policy: default-src 'none';\r\n"
             "X-Frame-Options: allow-from https://www.onlineconverter.com/\r\n"
             "X-XSS-Protection: 1; mode=block\r\n";

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
    // uint8_t * buf = NULL;
    // try
    // {
    //      buf = new uint8_t[100];
    // }
    // catch(std::bad_alloc & memExp)
    // {
    //     Serial.println("new fail");
    //     return;
    // }
    // uint8_t *buf = (uint8_t *)malloc(147978);
    // if (!buf)
    // {
    //     Serial.println("malloc fail");
    //     return;
    // }
    // Serial.println("new success");
    // delete[] buf;
    // free(buf);
    // return;
    // https://hostpro.onlineconverter.com/file/1064b75ef1afb9f97c36655bf427f43803/download

    // String part2 = "https://www.onlineconverter.com/convert/1006d2d318de0cacf322902dfa0f9ce2ee";
    // int i = part2.lastIndexOf("/");
    // part2 = part2.substring(i);
    // Serial.println(part2);
    // return;

    // String str1 = "2c\r\n"
    //               "https://www.onlineconverter.com/convert/busy\r\n"
    //               "0\r\n";

    // int start = str1.indexOf("https:");

    // str1 = str1.substring(start);
    // str1 =  str1.substring(0,str1.indexOf("\r\n"));
    // Serial.println("----------path--------");
    // Serial.println(str1);
    // return;

    // int start = str.indexOf("Content-Length:");

    // str = str.substring(start);

    // Serial.println(str);

    // start = str.indexOf(" ");
    // int end = str.indexOf("\r\n");

    // Serial.println(start);
    // Serial.println(end);

    // Serial.println(str.substring(0, end));
    // Serial.println("----------------\r\n");
    // Serial.println(str.substring(start, end).toInt());

    // String part2 = "https://www.onlineconverter.com/convert/10c6bebea623fb860389dfadddeca2ebea";

    // Serial.println(part2.substring(part2.lastIndexOf("/")));

    // int i = part2.indexOf("/", String("https://").length());

    // part2 = part2.substring(i);

    // Serial.println(part2);

    // return;

    if (!FILESYSTEM.begin())
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

    listDir(FILESYSTEM, "/", 2);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }
    Serial.println(F("WiFi connected"));
    Serial.println("");
    Serial.println(WiFi.localIP());

    // delay(500);

    // photoWeb.addToFile();
    // photoWeb.removeUrlFile();

    photoWeb.login(USER_NAME, PASSWORD);
    // server.on("/bmp", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     Serial.println("BMP");
    //     request->send(FILESYSTEM, "/TIM.bmp", "image/bmp");
    // });

    // server.onNotFound([](AsyncWebServerRequest *request) {
    //     request->send(404, "text/plain", "Not found");
    // });

    // server.begin();

    // File file = FILESYSTEM.open("/c.jpg");
    // if (!file)
    // {
    //     Serial.println("Open Fial -->");
    //     return;
    // }
    // Serial.print("filesize: ");
    // Serial.println(file.size());

    // uint8_t *p = (uint8_t *)malloc(file.size());
    // uint8_t *p = (uint8_t *)heap_caps_malloc(file.size(),MALLOC_CAP_SPIRAM);
    // if (!p)
    // {
    //     Serial.println("malloc fail");
    //     return;
    // }

    // size_t size = file.read(p, file.size());

    // if (size != file.size())
    // {
    //     Serial.println("Read fail");
    //     return;
    // }

    // file.close();

    // online_converter(p, size, 100, 80);

    // free(p);
}

void loop()
{
    delay(30000);
}