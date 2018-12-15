/**The MIT License (MIT)
Copyright (c) 2015 by Daniel Eichhorn
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
See more at http://blog.squix.ch

Adapted by Bodmer to use the faster TFT_eSPI library:
https://github.com/Bodmer/TFT_eSPI

Bodmer: Functions no longer needed weeded out, Jpeg decoder functions added
Bodmer: drawBMP() updated to buffer in and out pixels and use screen CGRAM rotation for faster bottom up drawing (now ~2x faster)
*/

#include "GfxUi.h"

GfxUi::GfxUi(TFT_eSPI *tft)
{
  _tft = tft;
  // _dispWin.x1 = 0;
  // _dispWin.y1 = 0;
  // _dispWin.x2 = _tft->width();
  // _dispWin.y2 = _tft->height();
}

void GfxUi::drawProgressBar(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint8_t percentage, uint16_t frameColor, uint16_t barColor)
{
  if (percentage == 0)
  {
    _tft->fillRoundRect(x0, y0, w, h, 3, TFT_BLACK);
  }
  uint8_t margin = 2;
  uint16_t barHeight = h - 2 * margin;
  uint16_t barWidth = w - 2 * margin;
  _tft->drawRoundRect(x0, y0, w, h, 3, frameColor);
  _tft->fillRect(x0 + margin, y0 + margin, barWidth * percentage / 100.0, barHeight, barColor);
}

// This drawBMP function contains code from:
// https://github.com/adafruit/Adafruit_ILI9341/blob/master/examples/spitftbitmap/spitftbitmap.ino
// Here is Bodmer's version: this uses the ILI9341 CGRAM coordinate rotation features inside the display and
// buffers both file and TFT pixel blocks, it typically runs about 2x faster for bottom up encoded BMP images

//void GfxUi::drawBMP(String filename, uint8_t x, uint16_t y, boolean flip) { // Alernative for caller control of flip
void GfxUi::drawBmp(String filename, uint8_t x, uint16_t y)
{
  // Flips the TFT internal SGRAM coords to draw bottom up BMP images faster, in this application it can be fixed
  boolean flip = 1;

  if ((x >= _tft->width()) || (y >= _tft->height()))
    return;

  fs::File bmpFile;
  int16_t bmpWidth, bmpHeight;        // Image W+H in pixels
  uint32_t bmpImageoffset;            // Start address of image data in file
  uint32_t rowSize;                   // Not always = bmpWidth; may have padding
  uint8_t sdbuffer[3 * BUFFPIXEL];    // file read pixel buffer (8 bits each R+G+B per pixel)
  uint16_t tftbuffer[BUFFPIXEL];      // TFT pixel out buffer (16-bit per pixel)
  uint8_t rgb_ptr = sizeof(sdbuffer); // read 24 bit RGB pixel data buffer pointer (8 bit so BUFF_SIZE must be less than 86)
  boolean goodBmp = false;            // Flag set to true on valid header parse
  int16_t w, h, row, col;             // to store width, height, row and column
  uint8_t rotation;                   // to restore rotation
  uint8_t tft_ptr = 0;                // TFT 16 bit 565 format pixel data buffer pointer

  // Check file exists and open it
  Serial.println(filename);
  if (!(bmpFile = FILESYSTEM.open(filename, "r")))
  {
    Serial.println(F(" File not found")); // Can comment out if not needed
    return;
  }

  // Parse BMP header to get the information we need
  if (read16(bmpFile) == 0x4D42)
  {                                   // BMP file start signature check
    read32(bmpFile);                  // Dummy read to throw away and move on
    read32(bmpFile);                  // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    read32(bmpFile);                  // Dummy read to throw away and move on
    bmpWidth = read32(bmpFile);       // Image width
    bmpHeight = read32(bmpFile);      // Image height

    // Only proceed if we pass a bitmap file check
    // Number of image planes -- must be '1', depth 24 and 0 (uncompressed format)
    if ((read16(bmpFile) == 1) && (read16(bmpFile) == 24) && (read32(bmpFile) == 0))
    {
      goodBmp = true; // Supported BMP format
      // BMP rows are padded (if needed) to 4-byte boundary
      rowSize = (bmpWidth * 3 + 3) & ~3;
      // Crop area to be loaded
      w = bmpWidth;
      h = bmpHeight;

      // We might need to alter rotation to avoid tedious file pointer manipulation
      // Save the current value so we can restore it later
      rotation = _tft->getRotation();
      // Use TFT SGRAM coord rotation if flip is set for 25% faster rendering (new rotations 4-7 supported by library)
      if (flip)
        _tft->setRotation((rotation + (flip << 2)) % 8); // Value 0-3 mapped to 4-7

      // Calculate new y plot coordinate if we are flipping
      switch (rotation)
      {
      case 0:
        if (flip)
          y = _tft->height() - y - h;
        break;
      case 1:
        y = _tft->height() - y - h;
        break;
        break;
      case 2:
        if (flip)
          y = _tft->height() - y - h;
        break;
        break;
      case 3:
        y = _tft->height() - y - h;
        break;
        break;
      }

      // Set TFT address window to image bounds
      // Currently, image will not draw or will be corrputed if it does not fit
      // TODO -> efficient clipping, but I don't need it to be idiot proof ;-)
      _tft->setAddrWindow(x, y, x + w - 1, y + h - 1);

      // Finally we are ready to send rows of pixels, writing like this avoids slow 32 bit multiply in 8 bit processors
      for (uint32_t pos = bmpImageoffset; pos < bmpImageoffset + h * rowSize; pos += rowSize)
      {
        // Seek if we need to on boundaries and arrange to dump buffer and start again
        if (bmpFile.position() != pos)
        {
          bmpFile.seek(pos, fs::SeekSet);
          rgb_ptr = sizeof(sdbuffer);
          //Serial.println("Seeking in file >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
        }

        // Fill the pixel buffer and plot
        for (col = 0; col < w; col++)
        { // For each column...
          // Time to read more pixel data?
          if (rgb_ptr >= sizeof(sdbuffer))
          {
            // Push tft buffer to the display
            if (tft_ptr)
            {
              // Here we are sending a uint16_t array to the function
              _tft->pushColors(tftbuffer, tft_ptr);
              tft_ptr = 0; // tft_ptr and rgb_ptr are not always in sync...
            }
            // Finally reading bytes from SD Card
            bmpFile.read(sdbuffer, sizeof(sdbuffer));
            rgb_ptr = 0; // Set buffer index to start
          }
          // Convert pixel from BMP 8+8+8 format to TFT compatible 16 bit word
          // Blue 5 bits, green 6 bits and red 5 bits (16 bits total)
          // Is is a long line but it is faster than calling a library fn for this
          tftbuffer[tft_ptr] = (sdbuffer[rgb_ptr++] >> 3);
          tftbuffer[tft_ptr] |= ((sdbuffer[rgb_ptr++] & 0xFC) << 3);
          tftbuffer[tft_ptr] |= ((sdbuffer[rgb_ptr++] & 0xF8) << 8);
          tft_ptr++;
        } // Next row
      }   // All rows done

      // Write any partially full buffer to TFT
      if (tft_ptr)
        _tft->pushColors(tftbuffer, tft_ptr);

    } // End of bitmap access
  }   // End of bitmap file check

  bmpFile.close();

  if (!goodBmp)
  {
    Serial.print(F("BMP format not recognised. File:"));
    Serial.println(filename);
  }
  else
    _tft->setRotation(rotation); // Put back original rotation
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t GfxUi::read16(fs::File &f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t GfxUi::read32(fs::File &f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

/*====================================================================================
  This sketch support functions to render the Jpeg images.

  Created by Bodmer 15th Jan 2017
  ==================================================================================*/

// Return the minimum of two values a and b
#define minimum(a, b) (((a) < (b)) ? (a) : (b))

#define USE_SPI_BUFFER // Comment out to use slower 16 bit pushColor()

//====================================================================================
//   Opens the image file and prime the Jpeg decoder
//====================================================================================
void GfxUi::drawJpeg(const char *filename, int xpos, int ypos)
{

  Serial.println("===========================");
  Serial.print("Drawing file: ");
  Serial.println(filename);
  Serial.println("===========================");

  // Open the named file (the Jpeg decoder library will close it after rendering image)
  // fs::File jpegFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
  File jpegFile = FILESYSTEM.open(filename, FILE_READ); // or, file handle reference for SD library

  if (!jpegFile)
  {
    Serial.print("ERROR: File \"");
    Serial.print(filename);
    Serial.println("\" not found!");
    return;
  }

  // Use one of the three following methods to initialise the decoder:

  int decoded = JpegDec.decodeSdFile(jpegFile); // or pass the SD file handle to the decoder,
  // int decoded = JpegDec.decodeFsFile(jpegFile); // Pass a SPIFFS file handle to the decoder,

  // boolean decoded = JpegDec.decodeFsFile(filename);  // or pass the filename (leading / distinguishes SPIFFS files)
  // Note: the filename can be a String or character array type

  Serial.printf("decoded : %d\n", decoded);
  if (decoded != -1)
  {
    // print information about the image to the serial port
    // jpegInfo();

    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else
  {
    Serial.println("Jpeg file format not supported!");
  }
}

//====================================================================================
//   Decode and render the Jpeg image onto the TFT screen
//====================================================================================
void GfxUi::jpegRender(int xpos, int ypos)
{

  // retrieve infomration about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
#ifdef USE_SPI_BUFFER
  while (JpegDec.readSwappedBytes())
  { // Swap byte order so the SPI buffer can be used
#else
  while (JpegDec.read())
  { // Normal byte order read
#endif
    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x)
      win_w = mcu_w;
    else
      win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y)
      win_h = mcu_h;
    else
      win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // draw image MCU block only if it will fit on the screen
    if ((mcu_x + win_w) <= _tft->width() && (mcu_y + win_h) <= _tft->height())
    {
      _tft->pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    }

    else if ((mcu_y + win_h) >= _tft->height())
      JpegDec.abort();
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print("Total render time was    : ");
  Serial.print(drawTime);
  Serial.println(" ms");
  Serial.println("=====================================");
}

//====================================================================================
//   Print information decoded from the Jpeg image
//====================================================================================
void GfxUi::jpegInfo()
{

  Serial.println("===============");
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}
//====================================================================================

#if 0
// User defined call-back function to input JPEG data from file
//---------------------
uint32_t tjd_s_input(JDEC *jd, BYTE *buff, UINT nd)
{
  int rb = 0;
  char recv_buf[16];
  // Device identifier for the session (5th argument of jd_prepare function)
  JPGIODEV *dev = (JPGIODEV *)jd->device;
  while (!dev->found_header)
  {
    rb += read(dev->s, recv_buf, 1);
    putchar(recv_buf[0]); // debug
    if (recv_buf[0] == '\r')
    {
      rb += read(dev->s, recv_buf, 3);
      putchar(recv_buf[0]);
      putchar(recv_buf[1]);
      putchar(recv_buf[2]); // debug
      if ((recv_buf[0] == '\n') && (recv_buf[1] == '\r') && (recv_buf[2] == '\n'))
      {
        ESP_LOGI("TFT", "Found header ended at %d", rb);
        dev->found_header = true;
      }
    }
  }

  if (buff)
  { // Read nd bytes from the input strem
    rb = read(dev->s, buff, nd);
    // ESP_LOGI("TFT", "Socket read %d, actual %d", nd, rb);
    return rb; // Returns actual number of bytes read
  }
  else
  { // Remove nd bytes from the input stream
    int seek_size = nd;
    while (seek_size)
    {
      if (seek_size > sizeof(recv_buf))
      {
        rb = read(dev->s, recv_buf, sizeof(recv_buf));
      }
      else
      {
        rb = read(dev->s, recv_buf, seek_size);
      }
      seek_size -= rb;
    }
    return nd; // Returns actual number of bytes read
               // if (fseek(dev->fhndl, nd, SEEK_CUR) >= 0) return nd;
               // else return 0;
  }
}

// User defined call-back function to input JPEG data from memory buffer
//-------------------------
uint32_t tjd_buf_input(JDEC *jd, BYTE *buff, UINT nd)
{
  // Device identifier for the session (5th argument of jd_prepare function)
  JPGIODEV *dev = (JPGIODEV *)jd->device;
  if (!dev->membuff)
    return 0;
  if (dev->bufptr >= (dev->bufsize + 2))
    return 0; // end of stream

  if ((dev->bufptr + nd) > (dev->bufsize + 2))
    nd = (dev->bufsize + 2) - dev->bufptr;

  if (buff)
  { // Read nd bytes from the input strem
    memcpy(buff, dev->membuff + dev->bufptr, nd);
    dev->bufptr += nd;
    return nd; // Returns number of bytes read
  }
  else
  { // Remove nd bytes from the input stream
    dev->bufptr += nd;
    return nd;
  }
}

// User defined call-back function to input JPEG data from file
//-------------------------
uint32_t GfxUi::inputFromFile(JDEC *jd, BYTE *buff, UINT nd)
{
  int rb = 0;
  // Device identifier for the session (5th argument of jd_prepare function)
  JPGIODEV *dev = (JPGIODEV *)jd->device;

  if (buff)
  { // Read nd bytes from the input strem
    rb = dev->fhndl.read(buff, nd);
    return rb; // Returns actual number of bytes read
  }
  else
  { // Remove nd bytes from the input stream
    if (dev->fhndl.seek(nd, SeekCur))
    {
      return nd;
    }
    else
    {
      if (_image_debug)
        log_e("seek fail");
      return 0;
    }
  }
}

uint32_t GfxUi::outputScreen(JDEC *jd, void *bitmap, JRECT *rect)
{
  // Device identifier for the session (5th argument of jd_prepare function)
  JPGIODEV *dev = (JPGIODEV *)jd->device;

  // ** Put the rectangular into the display device **
  int x;
  int y;
  int dleft, dtop, dright, dbottom;
  BYTE *src = (BYTE *)bitmap;

  int left = rect->left + dev->x;
  int top = rect->top + dev->y;
  int right = rect->right + dev->x;
  int bottom = rect->bottom + dev->y;

  if ((left > _dispWin.x2) || (top > _dispWin.y2))
    return 1; // out of screen area, return
  if ((right < _dispWin.x1) || (bottom < _dispWin.y1))
    return 1; // out of screen area, return

  if (left < _dispWin.x1)
    dleft = _dispWin.x1;
  else
    dleft = left;
  if (top < _dispWin.y1)
    dtop = _dispWin.y1;
  else
    dtop = top;
  if (right > _dispWin.x2)
    dright = _dispWin.x2;
  else
    dright = right;
  if (bottom > _dispWin.y2)
    dbottom = _dispWin.y2;
  else
    dbottom = bottom;

  if ((dleft > _dispWin.x2) || (dtop > _dispWin.y2))
    return 1; // out of screen area, return
  if ((dright < _dispWin.x1) || (dbottom < _dispWin.y1))
    return 1; // out of screen area, return

  uint32_t len = ((dright - dleft + 1) * (dbottom - dtop + 1)); // calculate length of data

  if ((len > 0) && (len <= JPG_IMAGE_LINE_BUF_SIZE))
  {
    uint8_t *dest = (uint8_t *)(dev->linbuf[dev->linbuf_idx]);

    for (y = top; y <= bottom; y++)
    {
      for (x = left; x <= right; x++)
      {
        // Clip to display area
        if ((x >= dleft) && (y >= dtop) && (x <= dright) && (y <= dbottom))
        {
          *dest++ = (*src++) & 0xFC;
          *dest++ = (*src++) & 0xFC;
          *dest++ = (*src++) & 0xFC;
        }
        else
          src += 3; // skip
      }
    }
    _tft->setAddrWindow(dleft, dtop, dright, dbottom);
    uint16_t *p = (uint16_t *)malloc(sizeof(uint16_t) * len);
    if (!p)
    {
      log_e("malloc fail");
      return 0;
    }
    for (int i = 0; i < len; i++)
    {
      p[i] = _tft->color565(dev->linbuf[dev->linbuf_idx][i].r, dev->linbuf[dev->linbuf_idx][i].g, dev->linbuf[dev->linbuf_idx][i].b);
    }
    _tft->pushColors(p, len);
    free(p);
    // log_i("x:%d y:%d x1:%d y1:%d len:%lu", dleft, dtop, dright, dbottom, len);
    dev->linbuf_idx = ((dev->linbuf_idx + 1) & 1);
  }
  else
  {
    log_e("Data size error: %d jpg: (%d,%d,%d,%d) disp: (%d,%d,%d,%d)\r\n", len, left, top, right, bottom, dleft, dtop, dright, dbottom);
    return 0; // stop decompression
  }

  return 1; // Continue to decompression
}

void GfxUi::drawJpeg(int x, int y, uint8_t scale, int s, char *fname, uint8_t *buf, int size)
{
  JPGIODEV dev;
  char *work = NULL;   // Pointer to the working buffer (must be 4-byte aligned)
  UINT sz_work = 3800; // Size of the working buffer (must be power of 2)
  JDEC jd;             // Decompression object (70 bytes)
  JRESULT rc;

  dev.linbuf[0] = NULL;
  dev.linbuf[1] = NULL;
  dev.linbuf_idx = 0;

  // dev.fhndl = NULL;
  dev.s = s;
  if (s >= 0)
  {
    // image from socket
    dev.membuff = NULL;
    dev.found_header = false;
    dev.bufsize = 0;
    dev.bufptr = 0;
  }
  else if (fname == NULL)
  {
    // image from buffer
    dev.membuff = buf;
    dev.bufsize = size;
    dev.bufptr = 0;
  }
  else
  {
    // image from file
    dev.membuff = NULL;
    dev.bufsize = 0;
    dev.bufptr = 0;
    if (!FILESYSTEM.exists(fname))
    {
      if (_image_debug)
        log_e("File error");
      goto exit;
    }

    dev.fhndl = FILESYSTEM.open(String(fname), "r");
    if (!dev.fhndl)
    {
      if (_image_debug)
        log_e("Error opening file");
      goto exit;
    }
    log_i("Open fname:%s", fname);
  }

  if (scale > 3)
    scale = 3;

  work = (char *)malloc(sz_work);
  if (work)
  {
    // if (dev.s >= 0)
    //   rc = jd_prepare(&jd, tjd_s_input, (void *)work, sz_work, &dev);
    // else if (dev.membuff)
    //   rc = jd_prepare(&jd, tjd_buf_input, (void *)work, sz_work, &dev);
    // else
    rc = jd_prepare(&jd, inputFromFile, (void *)work, sz_work, &dev);

    if (rc == JDR_OK)
    {
      log_i("decode size: %u * %u", jd.width, jd.height);
      if (x == CENTER)
        x = ((_dispWin.x2 - _dispWin.x1 + 1 - (int)(jd.width >> scale)) / 2) + _dispWin.x1;
      else if (x == RIGHT)
        x = _dispWin.x2 + 1 - (int)(jd.width >> scale);

      if (y == CENTER)
        y = ((_dispWin.y2 - _dispWin.y1 + 1 - (int)(jd.height >> scale)) / 2) + _dispWin.y1;
      else if (y == BOTTOM)
        y = _dispWin.y2 + 1 - (int)(jd.height >> scale);

      if (x < ((_dispWin.x2 - 1) * -1))
        x = (_dispWin.x2 - 1) * -1;
      if (y < ((_dispWin.y2 - 1)) * -1)
        y = (_dispWin.y2 - 1) * -1;
      if (x > (_dispWin.x2 - 1))
        x = _dispWin.x2 - 1;
      if (y > (_dispWin.y2 - 1))
        y = _dispWin.y2 - 1;

      dev.x = x;
      dev.y = y;

      dev.linbuf[0] = (color_t *)heap_caps_malloc(JPG_IMAGE_LINE_BUF_SIZE * 3, MALLOC_CAP_DMA);
      if (dev.linbuf[0] == NULL)
      {
        if (_image_debug)
          log_e("Error allocating line buffer #0\r\n");
        goto exit;
      }
      dev.linbuf[1] = (color_t *)heap_caps_malloc(JPG_IMAGE_LINE_BUF_SIZE * 3, MALLOC_CAP_DMA);
      if (dev.linbuf[1] == NULL)
      {
        if (_image_debug)
          log_e("Error allocating line buffer #1\r\n");
        goto exit;
      }

      // Start to decode the JPEG file
      rc = jd_decomp(&jd, outputScreen, scale);
      if (rc != JDR_OK)
      {
        if (_image_debug)
          log_e("jpg decompression error %d\r\n", rc);
      }
      if (_image_debug)
        log_i("Jpg size: %dx%d, position; %d,%d, scale: %d, bytes used: %d\r\n", jd.width, jd.height, x, y, scale, jd.sz_pool);
    }
    else
    {
      if (_image_debug)
        log_e("jpg prepare error %d\r\n", rc);
    }
  }
  else
  {
    if (_image_debug)
      log_e("work buffer allocation error\r\n");
  }

exit:
  if (work)
    free(work); // free work buffer
  if (dev.linbuf[0])
    free(dev.linbuf[0]);
  if (dev.linbuf[1])
    free(dev.linbuf[1]);
  if (dev.fhndl)
    dev.fhndl.close();
}
#endif
