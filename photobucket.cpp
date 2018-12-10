#include "photobucket.h"
#include "config.h"

#define HTTPS_HOST "secure.photobucket.com"
#define HTTPS_PORT 443

#define HTTP_HOST "s1268.photobucket.com"
#define HTTP_PORT 80

#define HTTP_PHOTO_HOST "rs1268.pbsrc.com"
#define HTTP_PHOTO_PORT 80

// WiFiClientSecure client;

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

uint32_t PHOTOBUCCKET::searchContent()
{
    char status[64] = {0};
    while (1)
    {
        readBytesUntil('\r', status, sizeof(status));
        // Serial.print("Search:");
        String str = String(status);
        // Serial.println(str);

        int index = str.indexOf("Content-Length:");
        if (index > 0)
        {
            int s, e;
            str = str.substring(index);
            s = str.indexOf(" ");
            e = str.indexOf("\r\n");
            log_i("Search content:%lu", str.substring(s, e).toInt());
            return str.substring(s, e).toInt();
        }
    }
    return 0;
}

bool PHOTOBUCCKET::sendGetRequest(const char *path)
{
    char status[32] = {0};
    // log_i("REQUEST PATH:%s\n", path);
    String packet = "GET " + String(path) + " HTTP/1.1\r\n";
    packet += "Host: rs1268.pbsrc.com\r\n";
    packet += "Connection: keep-alive\r\n";
    packet += "Upgrade-Insecure-Requests: 1\r\n";
    packet += "User-Agent: Mozilla/5.0 (Linux; Android 8.0.0; LG-US998) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.110 Mobile Safari/537.36\r\n";
    packet += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n";
    packet += "Accept-Encoding: deflate\r\n";
    packet += "Accept-Language: zh,zh-CN;q=0.9,en;q=0.8\r\n";
    println(packet);

    readBytesUntil('\r', status, sizeof(status));
    log_i("RESPONSE:%s", status);

    if (0 != strncmp(status, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK")))
    {
        log_e("GET REQUEST RESPONSE: %s\n", status);
        readString();
        // flush();
        return false;
    }
    return true;
}

bool PHOTOBUCCKET::downloadPhoto()
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

    //! test
    for (int i = 0; i < nums; ++i)
    {
        if (!connect(HTTP_PHOTO_HOST, HTTP_PHOTO_PORT))
        {
            log_e("CONNECT HOST FAIL");
            return false;
        }

        if (!connected())
        {
            log_e("LOST HOST CONNECT");
            return false;
        }
        String url = String((const char *)root[i]);

        int startIndex = url.lastIndexOf("/");
        if (startIndex < 0)
            return false;
        int endIndex = url.lastIndexOf("?");
        if (endIndex < 0)
            return false;
        String name = url.substring(startIndex, endIndex);

        log_i("URL:%s", url.c_str());

        if (name.endsWith(".gif"))
            continue;

        log_i("DOWNLOAD[%d] : %s", i, name.c_str());
        //Hander url path
        startIndex = url.indexOf("com") + 3;
        if (startIndex < 0)
            continue;
        url = url.substring(startIndex);

        //Send GET Requests
        if (!sendGetRequest(url.c_str()))
        {
            log_e("REQUEST ERROR");
            continue;
        }

        uint32_t len = searchContent();
        if (!len)
            return false;

        // skip html head
        if (!find("\r\n\r\n"))
        {
            log_e("NOT FIND BODY");
            readString();
            return false;
        }

        //! body is here
        //TODO Open file store photo to sd card

        log_i("OPEN [%s] Begin ...", name.c_str());

        // TODO : Determine if the file is duplicated
        // TODO ....
        f = FILESYSTEM.open(name, FILE_WRITE);
        if (!f)
        {
            log_e("OPEN FILE FAIL");
            return false;
        }

        uint64_t timeStamp = millis();
        size_t filesize = 0;
        while (1)
        {
            int r = read();
            if (r != -1)
            {
                f.write(r);
                ++filesize;
                timeStamp = millis();
            }
            if (millis() - timeStamp > 10 * 1000)
            {
                log_e("TIME OUT");
                break;
            }
        }
        log_i("FILE SIZE : %lu", filesize);
        f.close();
        stop();
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

bool PHOTOBUCCKET::parseUrl()
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

int PHOTOBUCCKET::searchIndex()
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
                log_i("Search Index : %d\n", index);
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

// TODO Need to optimize data reception
bool PHOTOBUCCKET::readConntent()
{
    uint64_t timeStamp = millis();
    json = "";

    // !
    char *str = (char *)heap_caps_malloc(100 * 1024, MALLOC_CAP_SPIRAM);
    if (!str)
    {
        log_e("MALLOC STR BUFFER FAIL");
        return false;
    }
    // !
    memset(str, 0, 100 * 1024);
    char *r = str;
    while (connected())
    {
        if (available())
        {
#if 0
            json += readString();
#else
            *r++ = (char)read();
            // str++;
            // Serial.print((char)read());
#endif
            timeStamp = millis();
        }
        if ((millis() - timeStamp) > 30000)
        {
            log_e("[TIME OUT]");
            break;
        }
    }
    //!

    const char *found = strstr(str, ",\"currentAlbumPath\"");
    if (found == NULL)
    {
        log_e("CAN'T FIND STR");
        return false;
    }
    int index = found - str;
    Serial.printf("Search : %d\n", index);
    str[index] = '}';
    str[index + 1] = '\0';
    Serial.printf("%s\n", str);
    free(str);
    // return false;
    //!
    return true;
}

bool PHOTOBUCCKET::login(const char *username, const char *password)
{
    char status[64];
    String packet = "GET /user/" + String(username) + "/library/ HTTP/1.1\r\n";
    packet += "Host: s1268.photobucket.com\r\n";
    packet += "Connection: keep-alive\r\n";
    packet += "Pragma: no-cache\r\n";
    packet += "Cache-Control: no-cache\r\n";
    packet += "Upgrade-Insecure-Requests: 1\r\n";
    packet += "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.75 Safari/537.36\r\n";
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

    readBytesUntil('\r', status, sizeof(status));
    log_i("RESPONSE:%s", status);
    if (0 != strncmp(status, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK")))
    {
        stop();
        return false;
    }

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
        log_e("DATA INVALUE");
        stop();
        return false;
    }
    return parseUrl();
}
