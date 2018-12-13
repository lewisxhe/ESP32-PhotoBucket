#include "photobucket.h"
#include "config.h"

#define HTTPS_HOST "secure.photobucket.com"
#define HTTPS_PORT 443

#define HTTP_HOST "s1268.photobucket.com"
#define HTTP_PORT 80

#define HTTP_PHOTO_HOST "rs1268.pbsrc.com"
#define HTTP_PHOTO_PORT 80

uint16_t PHOTOBUCCKET::getUrlNums()
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

bool PHOTOBUCCKET::getFileNameByUrl(const char *url)
{
    _FileName = String(url);
    int s, e;
    s = _FileName.lastIndexOf("/");
    e = _FileName.lastIndexOf("?");
    if (s < 0 || e < 0)
    {
        return false;
    }
    _FileName = _FileName.substring(s, e);
    return true;
}

bool PHOTOBUCCKET::isFileValid()
{
    File f = FILESYSTEM.open(_FileName, FILE_READ);
    if (!f || !f.size())
        return false;
    f.close();
    return true;
}

bool PHOTOBUCCKET::downloadPhoto(ProgressCallback progressCallback)
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

    for (int i = 0; i < nums; ++i)
    {
        String url = String((const char *)root[i]);

        if (
            !getFileNameByUrl(url.c_str()) || isFileValid() ||
#ifdef DISABLE_GIF_DOWNLOAD
            _FileName.endsWith(".gif")
#endif
        )
        {
            stop();
            continue;
        }

        downloadFile(url, _FileName, progressCallback);
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

void PHOTOBUCCKET::removeUrlFile()
{
    if (FILESYSTEM.exists(DATABASE_FILENAME))
    {
        FILESYSTEM.remove(DATABASE_FILENAME);
    }
}

bool PHOTOBUCCKET::parseUrl(const char *json)
{

    log_i("SRAM:%lu\n", ESP.getPsramSize());

    log_i("ParseObject len : %u\n", strlen(json));

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
        delete[] sameArray;
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
    // reset jsonBuffer
    jsonBuffer.clear();
    JsonObject &sizeJson = jsonBuffer.createObject();
    sizeJson["size"] = rootArray->size();
    sizeJson.printTo(Serial);
    sizeJson.printTo(f);
    f.close();
    return true;
}

// http://s1268.photobucket.com/user/Z398507699lf/library?page=2

bool parseHtml()
{
}

bool PHOTOBUCCKET::login(const char *username, const char *password)
{
    bool ret;
    File f;

    String url = "http://s1268.photobucket.com/user/" + String(username) + "/library/";
    String filename = "/index.html";

    if (!downloadFile(url, filename))
    {
        log_e("DOWNLOAD FAIL");
        return false;
    }

    f = FILESYSTEM.open(filename, "r");
    if (!f)
    {
        log_e("OPEN FAIL");
        return false;
    }

    int r = f.find("collectionData:");
    if (r <= 0)
    {
        log_e("CAN'T FIND collectionData");
        f.close();
        return false;
    }
    size_t cur = f.position();

    log_i("cur position: %lu\n", cur);

    String tag = ",\"currentAlbumPath\"";

    r = f.find(tag.c_str());
    if (r <= 0)
    {
        log_e("CAN'T FIND currentAlbumPath");
        f.close();
        return false;
    }
    size_t now = f.position() - tag.length() + 2;

    log_i("now position: %lu\n", now);

    f.seek(cur);

    size_t bufferSize = now - cur;
    _buffer = (uint8_t *)heap_caps_realloc(_buffer, bufferSize, MALLOC_CAP_SPIRAM);
    if (!_buffer)
    {
        log_e("MALLOC STR BUFFER FAIL");
        f.close();
        return false;
    }
    memset(_buffer, 0, bufferSize);

    f.read(_buffer, bufferSize);

    f.close();

    _buffer[bufferSize - 2] = '}';
    _buffer[bufferSize - 1] = '\0';

    log_i("Buffer Size:%lu\n", bufferSize);

    ret = parseUrl((char *)_buffer);

    free((void *)_buffer);

    return ret;
}

bool PHOTOBUCCKET::downloadFile(String url, String filename)
{
    return downloadFile(url, filename, nullptr);
}

bool PHOTOBUCCKET::downloadFile(String url, String filename, ProgressCallback progressCallback)
{
    HTTPClient http;

    log_i("Downloading %s from %s", filename.c_str(), url.c_str());

    http.begin(url);

    int httpCode = http.GET();

    log_i("[HTTP] RESPONSE: %d\n", httpCode);

    if (httpCode <= 0)
    {
        log_e("[HTTP] GET FIAL, ERROR: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    File f = FILESYSTEM.open(filename, FILE_WRITE);
    if (!f)
    {
        log_e("FILE OPEN FIAL");
        http.end();
        return false;
    }

    if (httpCode == HTTP_CODE_OK)
    {
        uint8_t buff[128] = {0};
        int total = http.getSize();
        int len = total;
        if (progressCallback)
            progressCallback(filename, 0, total);

        WiFiClient *stream = http.getStreamPtr();

        while (http.connected() && (len > 0 || len == -1))
        {
            size_t size = stream->available();
            if (size)
            {
                int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                f.write(buff, c);

                if (len > 0)
                {
                    len -= c;
                }
                if (progressCallback)
                {
                    // log_i("%d / %d \n", total - len, total);
                    progressCallback(filename, total - len, total);
                }
            }
            delay(1);
        }
        log_i("[HTTP] connection closed or file end.\n");
    }
    f.close();
    http.end();
    return true;
}
