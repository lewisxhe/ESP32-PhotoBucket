
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
  PHOTOBUCCKET(String userName){_userName = userName;};
  bool getMainPage();
  bool downloadPhoto(ProgressCallback progressCallback);
  void removeUrlFile();
  bool jumpPage(int page);
  bool parseHtml();
  bool parseJSON(int &pages);
  void testGET();

private:
  uint16_t getUrlNums();
  bool searchSameUrl(JsonArray &array, const char *url);
  bool getFileNameByUrl(const char *url);
  bool isFileValid();
  bool downloadFile(String url, String filename);
  bool downloadFile(String url, String filename, ProgressCallback progressCallback);

protected:
  String _dateBaseFileName = "/index.html";
  String _userName;
  String _FileName;
  uint8_t *_buffer = NULL;
};