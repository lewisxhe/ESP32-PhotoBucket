
#pragma onec
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// #define ENABLE_PRINT_SRC_JSON_BUFFER
#define DISABLE_GIF_DOWNLOAD

#define DATABASE_FILENAME "/photo.json"
#define DATABASE_JSON_ARRAY_SIZE_FILE "/photoSize.json"

typedef void (*ProgressCallback)(String fileName, uint32_t bytesDownloaded, uint32_t bytesTotal);

class PHOTOBUCCKET : public WiFiClient
{
public:
  bool login(const char *username, const char *password);
  bool downloadPhoto(ProgressCallback progressCallback);
  void removeUrlFile();

private:
  uint16_t getUrlNums();
  bool parseUrl(const char *json);
  bool searchSameUrl(JsonArray &array, const char *url);
  bool getFileNameByUrl(const char *url);
  bool isFileValid();
  bool downloadFile(String url, String filename);
  bool downloadFile(String url, String filename,ProgressCallback progressCallback);

protected:
  String _FileName;
  uint8_t *_buffer = NULL;
};