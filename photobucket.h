
#pragma onec
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// #define ENABLE_PRINT_SRC_JSON_BUFFER

#define DATABASE_FILENAME "/photo.json"
#define DATABASE_JSON_ARRAY_SIZE_FILE "/photoSize.json"

class PHOTOBUCCKET : public WiFiClient
{
public:
  bool login(const char *username, const char *password);
  bool downloadPhoto();
  void removeUrlFile();

private:
  uint16_t getUrlNums();
  bool sendGetRequest(const char *path);
  int searchIndex();
  bool readConntent();
  bool parseUrl();
  bool searchSameUrl(JsonArray &array, const char *url);
  uint32_t searchContent();

      protected : char status[64];
  bool isConnect;
  String json;
};