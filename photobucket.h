

#include <WiFi.h>
#include <WiFiClientSecure.h>

// #define ENABLE_PRINT_SRC_JSON_BUFFER
#include <ArduinoJson.h>

#define DATABASE_FILENAME "/photo.json"
#define DATABASE_JSON_ARRAY_SIZE_FILE "/photoSize.json"

class PHOTOBUCCKET : public WiFiClient 
{
public:
  bool login(const char *username, const char *password);
  bool getHash(void);
  void getPictureJson(void);
  bool downloadPhoto(void);
  bool addToFile(void);
  void removeUrlFile(void);

private:
  uint16_t getUrlNums(void);
  bool sendGetRequest(const char *path);
  int searchIndex(void);
  bool readConntent(void);
  bool parseUrl(void);
  bool searchSameUrl(JsonArray &array, const char *url);

protected:
  char status[64];
  bool isConnect;
  String json;
};