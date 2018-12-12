
#pragma onec
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// #define ENABLE_PRINT_SRC_JSON_BUFFER
#define DISABLE_GIF_DOWNLOAD

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
  const char *readConntent(); /*Read Content ,return String pointer ,need manual free*/
  bool parseUrl(const char *json);
  bool searchSameUrl(JsonArray &array, const char *url);
  uint32_t searchContent();
  bool getFileNameByUrl(const char *url);
  bool isFileValid();
  bool getUrlPath(String &path);
  bool downloadFile(String url, String filename);

protected:
  // char status[64];
  String _path;
  String _FileName;
  String json;
  uint8_t *_buffer = NULL;
};