
#pragma onec
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// #define ENABLE_PRINT_SRC_JSON_BUFFER
// #define ENABKE_PRINT_FILE_JSON
#define DISABLE_GIF_DOWNLOAD

#define DATABASE_FILENAME "/photo.json"
#define DATABASE_JSON_ARRAY_SIZE_FILE "/photoSize.json"

typedef void (*ProgressCallback)(String fileName, uint32_t bytesDownloaded, uint32_t bytesTotal);

class PHOTOBUCCKET : public WiFiClient
{
public:
  PHOTOBUCCKET(String userName) { _userName = userName; };
  ~PHOTOBUCCKET() {}
  bool getMainPage();
  bool downloadPhoto();
  void removeUrlFile();
  bool jumpPage(int page);
  bool parseHTML();
  bool parseJSON();
  void testGET();
  void setProgressCallback(ProgressCallback progressCallback) { _progressCallback = progressCallback; }

private:
  uint16_t searchPageTotal();
  uint16_t getUrlNums();
  bool searchSameUrl(JsonArray &array, const char *url);
  bool getFileNameByUrl(const char *url);
  bool isFileValid();
  bool downloadFile(String url, String filename);

protected:
  ProgressCallback _progressCallback = NULL;
  String _dateBaseFileName = "/index.html";
  String _userName;
  String _FileName;
  uint8_t *_buffer = NULL;
};