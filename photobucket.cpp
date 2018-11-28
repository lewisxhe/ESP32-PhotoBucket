
#include <FS.h>
#include <SPIFFS.h>
#include "photobucket.h"

#define FILESYSTEM SPIFFS
#define HTTPS_HOST "secure.photobucket.com"
#define HTTPS_PORT 443

#define HTTP_HOST "s1268.photobucket.com"
#define HTTP_PORT 80

#define HTTP_PHOTO_HOST "rs1268.pbsrc.com"
#define HTTP_PHOTO_PORT 80

// WiFiClientSecure client;

bool PHOTOBUCCKET::getHash(void)
{
    return true;
}

uint16_t PHOTOBUCCKET::getUrlNums(void)
{
    File f = FILESYSTEM.open(DATABASE_JSON_ARRAY_SIZE_FILE, FILE_READ);
    if (!f)
    {
        log_e("OPEN FILE FAIL");
        return 0;
    }
    DynamicJsonBuffer jsonBuffer(128);
    JsonObject &size = jsonBuffer.parseObject(f);
    if (!size.success())
    {
        return 0;
    }
    return size.get<uint32_t>("size");
}

bool PHOTOBUCCKET::sendGetRequest(const char *path)
{
    char status[32] = {0};
    log_i("REQUEST PATH:%s\n", path);
    String packet = "GET " + String(path) + " HTTP/1.1\r\n";
    packet += "Host: rs1268.pbsrc.com\r\n";
    packet += "Connection: keep-alive\r\n";
    packet += "Upgrade-Insecure-Requests: 1\r\n";
    packet += "User-Agent: Mozilla/5.0 (Linux; Android 8.0.0; LG-US998) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.110 Mobile Safari/537.36\r\n";
    packet += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n";
    packet += "Accept-Encoding: deflate\r\n";
    packet += "Accept-Language: zh,zh-CN;q=0.9,en;q=0.8\r\n";
    println(packet);

    // Serial.println(readString());
    // return false;

    readBytesUntil('\r', status, sizeof(status));
    Serial.print("response:");
    Serial.println(status);

    if (0 != strncmp(status, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK")))
    {
        log_e("GET REQUEST RESPONSE: %s\n", status);
        readString();
        return false;
    }

    if (!find("\r\n\r\n"))
    {
        log_e("NOT FIND BODY");
        readString();
        return false;
    }
    return true;
}

bool PHOTOBUCCKET::downloadPhoto(void)
{

    uint16_t nums = getUrlNums();
    log_i("SEARCH URL:%u\n", nums);
    if (!nums)
    {
        return false;
    }

    // 80kb
    size_t bufferSize = JSON_ARRAY_SIZE(nums) + nums * JSON_OBJECT_SIZE(1);
    DynamicJsonBuffer jsonBuffer(bufferSize);

    File f = FILESYSTEM.open(DATABASE_FILENAME, FILE_READ);
    if (!f)
    {
        log_e("OPEN FILE FAIL");
        return false;
    }
    String jsonString = f.readString();
    f.close();

    JsonArray &root = jsonBuffer.parseArray(jsonString);
    if (!root.success())
    {
        log_e("PARSER JSON FAIL");
        return false;
    }
    //!!  ////////////////////////////////////////////////////////////////////////////!!!!!

    if (!connect(HTTP_PHOTO_HOST, HTTP_PHOTO_PORT))
    {
        log_e("CONNECT HOST FAIL");
        return false;
    }

    // for (int i = 0; i < nums; ++i)
    for (int i = 0; i < 2; ++i)
    {
        log_i("-----------------------\r\n");
        if (!connected())
        {
            log_e("LOST HOST CONNECT");
            return false;
        }
        //Hander url path
        String url = String((const char *)root[i]);
        int f = url.indexOf("com") + 3;
        if (f < 0)
            continue;
        url = url.substring(f);
        //Send GET Requests
        if (!sendGetRequest(url.c_str()))
        {
            log_e("REQUEST ERROR");
            continue;
        }
        else
        {
            // Serial.println(readString());
        }
        //Open file store photo to sd card
    }

    return true;
}

int PHOTOBUCCKET::searchIndex(void)
{
    int index = -1;
    uint64_t timeStamp = millis();
    while (connected())
    {
        if (available())
        {
            timeStamp = millis();
            index = find("collectionData:");
            if (index > 0)
            {
                log_i("SCAREN Index : %d\n", index);
                break;
            }
        }
        if ((millis() - timeStamp) > 15000)
        {
            log_e("[TIME OUT]");
            break;
        }
    }
    return index;
}

bool PHOTOBUCCKET::readConntent(void)
{
    uint64_t timeStamp = millis();
    json = "";
    while (connected())
    {
        if (available())
        {
            json += readString();
            timeStamp = millis();
        }
        if ((millis() - timeStamp) > 30000)
        {
            log_e("[TIME OUT]");
            break;
        }
    }
    return true;
}

bool PHOTOBUCCKET::searchSameUrl(JsonArray &array, const char *url)
{
    for (int i = 0; i < array.size(); ++i)
    {
        if (!strcmp(url, (const char *)array[i]))
            return true;
    }
    return false;
}

void PHOTOBUCCKET::removeUrlFile(void)
{
    if (FILESYSTEM.exists(DATABASE_FILENAME))
    {
        FILESYSTEM.remove(DATABASE_FILENAME);
    }
}

bool PHOTOBUCCKET::parseUrl(void)
{

    log_i("SRAM:%lu\n", ESP.getPsramSize());

    log_i("ParseObject len : %u\n", json.length());
    size_t bufferSize = JSON_ARRAY_SIZE(24) + 24 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 24 * JSON_OBJECT_SIZE(47) + (80 * 1024);
    DynamicJsonBuffer jsonBuffer(bufferSize);

    JsonObject &root = jsonBuffer.parseObject(json);
    if (!root.success())
    {
        log_i("Parsing failed!");
        return false;
    }
    log_i("Parsing Success");

    log_i("SRAM:%lu\n", ESP.getPsramSize());

    int pageSize = root["pageSize"];
    JsonObject &items = root["items"];
    int items_total = items["total"];

    JsonArray &items_objects = items["objects"];

    log_i("---------Information--------------");
    log_i("pageSize     (%d)", pageSize);
    log_i("items_total  (%d)", items_total);
    log_i("objects size (%u)", items_objects.size());
    log_i("--------------END-----------------");

    if (!items_objects.size())
    {
        log_i("NO DATA ...");
        return false;
    }

    int arrayObjects = items_objects.size();
    log_i("SRAM:%lu\n", ESP.getPsramSize());

    //Create Array
    bufferSize = JSON_ARRAY_SIZE(arrayObjects) + arrayObjects * JSON_OBJECT_SIZE(1);
    DynamicJsonBuffer arrayBuffer(bufferSize);
    JsonArray *rootArray = nullptr;

    log_i("SRAM:%lu\n", ESP.getPsramSize());

    File f;
    bool exists = false;
    if (FILESYSTEM.exists(DATABASE_FILENAME))
    {
        exists = true;
        f = FILESYSTEM.open(DATABASE_FILENAME, FILE_READ);
        if (!f)
        {
            log_e("OPEN FILE FAIL");
            return false;
        }
        rootArray = &arrayBuffer.parseArray(f.readString());
        if (!rootArray->success())
        {
            log_e("PARSER ARRAY BUFFER FAIL");
            return false;
        }

        bool *sameArray = nullptr;
        try
        {
            sameArray = new bool[arrayObjects]();
        }
        catch (std::bad_alloc)
        {
            log_e("NEW ARRAY BUFFER FAIL");
            return false;
        }

        //record same url subscript
        for (int i = 0; i < arrayObjects; ++i)
        {
            const char *url = items_objects[i]["mobileFullsizeUrl"];
            sameArray[i] = !searchSameUrl(*rootArray, url);
        }

        // add diff url to array
        for (int i = 0; i < arrayObjects; ++i)
        {
            if (sameArray[i])
            {
                const char *url = items_objects[i]["mobileFullsizeUrl"];
                log_i("add url:%s\n", url);
                if (!rootArray->add(url))
                {
                    log_e("ADD ARRAY OBJECT FAIL");
                    return false;
                }
            }
        }
        f.close();

        //store to file
        f = FILESYSTEM.open(DATABASE_FILENAME, FILE_WRITE);
        if (!f)
        {
            log_e("OPEN FILE FAIL");
            return false;
        }
        rootArray->printTo(f);
        f.close();
    }
    else
    {
        f = FILESYSTEM.open(DATABASE_FILENAME, FILE_WRITE);
        if (!f)
        {
            log_e("OPEN FILE FAIL");
            return false;
        }
        rootArray = &arrayBuffer.createArray();
        if (!rootArray->success())
        {
            log_e("CREARTE ARRAY BUFFER FAIL");
            return false;
        }
        //Traversing json array
        for (int i = 0; i < arrayObjects; i++)
        {
            if (!rootArray->add((const char *)items_objects[i]["mobileFullsizeUrl"]))
            {
                log_e("ADD ARRAY OBJECT FAIL");
                return false;
            }
        }
        rootArray->printTo(f);
        f.close();
    }
    log_i("---------Information--------------");
    log_i("objects size (%u)", rootArray->size());
    log_i("--------------END-----------------");
    Serial.println();
    rootArray->prettyPrintTo(Serial);
    Serial.println();

    f = FILESYSTEM.open(DATABASE_JSON_ARRAY_SIZE_FILE, FILE_WRITE);
    if (!f)
    {
        log_e("OPEN FILE FAIL");
        return false;
    }
    // reset buffer
    jsonBuffer.clear();
    JsonObject &sizeJson = jsonBuffer.createObject();
    sizeJson["size"] = rootArray->size();
    sizeJson.printTo(Serial);
    sizeJson.printTo(f);
    f.close();
    return true;
}

bool PHOTOBUCCKET::login(const char *username, const char *password)
{
    String packet = "GET /user/" + String(username) + "/library/ HTTP/1.1\r\n";
    packet += "Host: s1268.photobucket.com\r\n";
    packet += "Connection: keep-alive\r\n";
    packet += "Upgrade-Insecure-Requests: 1\r\n";
    packet += "User-Agent: Mozilla/5.0 (Linux; Android 8.0.0; LG-US998) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.80 Mobile Safari/537.36\r\n";
    packet += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n";
    packet += "Referer: http://www.photobucket.com/\r\n";

#ifdef USE_GZIP
    packet += "Accept-Encoding: gzip, deflate\r\n";
#else
    packet += "Accept-Encoding: deflate\r\n";
#endif
    packet += "Accept-Language: zh,zh-CN;q=0.9,en;q=0.8\r\n";
    packet += "\r\n\r\n";

    if (!connect(HTTP_HOST, HTTP_PORT))
    {
        log_e("CONNECT HOST FAIL");
        return false;
    }

    print(packet);

    int index = searchIndex();
    if (index < 0)
    {
        stop();
        return false;
    }

    readConntent();

    index = json.indexOf(",\"currentAlbumPath\"");
    if (index > 0)
    {
        log_i("DATA SCREAM RESULT:");
        json = json.substring(0, index);
        json += "}";
#ifdef ENABLE_PRINT_SRC_JSON_BUFFER
        Serial.println(json);
#endif
        stop();
    }
    else
    {
#ifdef ENABLE_PRINT_SRC_JSON_BUFFER
        Serial.println(json);
#endif
        log_e("DATA NOT INVALUE");
        stop();
        return false;
    }
    return parseUrl();
}

void PHOTOBUCCKET::getPictureJson(void)
{

    if (!isConnect)
        return;
    String packet = "GET /user/Z398507699lf/library/ HTTP/1.1\r\n";
    packet += "Host: s1268.photobucket.com\r\n";
    packet += "Connection: keep-alive\r\n";
    packet += "Upgrade-Insecure-Requests: 1\r\n";
    packet += "User-Agent: Mozilla/5.0 (Linux; Android 8.0.0; LG-US998) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.80 Mobile Safari/537.36\r\n";
    packet += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n";
    packet += "Referer: http://www.photobucket.com\r\n";
    packet += "Accept-Encoding: gzip, deflate\r\n";
    packet += "Accept-Language: zh,zh-CN;q=0.9,en;q=0.8packet += \r\n";
    packet += "\r\n\r\n";

    print(packet);

    bzero(status, sizeof(status));

    Serial.println(readString());
    return;

    readBytesUntil('\r', status, sizeof(status));
    Serial.println(status);

    if (strncmp(status, "HTTP/1.1 302 Found", strlen("HTTP/1.1 302 Found")))
    {
        Serial.print("Login Unexpected response: ");
        Serial.println(status);
        isConnect = false;
        stop();
        return;
    }
}

#if 0
#ifdef USE_GZIP
    char *path = "/index.tar.gz";
#else
    char *path = "/index.html"
#endif
    int rBytes;
    int wBytes = 1024;
    uint8_t buffer[wBytes];
    File f = FILESYSTEM.open(path, FILE_WRITE);
    if (!f)
    {
        log_e("[OPEN FILESYSTEM FAIL]");
        stop();
        return false;
    }
#endif

// String packet = "POST /action/auth/login HTTP/1.1\r\n";
// packet += "Host: secure.photobucket.com\r\n";
// packet += "Connection: keep-alive\r\n";
// packet += "Content-Length: " + String(body.length()) + "\r\n";
// packet += "Cache-Control: max-age=0\r\n";
// packet += "Origin: https://secure.photobucket.com\r\n";
// packet += "Upgrade-Insecure-Requests: 1\r\n";
// packet += "User-Agent: Mozilla/5.0 (Linux; Android 8.0.0; LG-US998) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.80 Mobile Safari/537.36\r\n";
// packet += "Content-Type: application/x-www-form-urlencoded\r\n";
// packet += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n";
// packet += "Referer: https://secure.photobucket.com/login\r\n";
// packet += "Accept-Encoding: gzip, deflate, br\r\n";
// packet += "Accept-Language: zh,zh-CN;q=0.9,en;q=0.8\r\n";
// packet += "\r\n\r\n";