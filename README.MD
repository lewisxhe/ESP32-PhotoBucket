ESP32 / POOTOBUCKET 
=====================

- This program uses ESP32 to capture the picture of [Photobucket](http://photobucket.com/) and display it on the display.
  
- By default, [JPEGDecoder](https://github.com/Bodmer/JPEGDecoder) is used to decode the image. It does not scale the image to fit the screen size. I will look for a better way to adapt the image to the screen size.
  
- After the image download is completed, the downloaded image will not be displayed. Although the download and display can be synchronized, but it is not safe, the downloaded image should be recorded in a separate folder, and the downloaded image will be displayed after the download is completed.

- The default JSON parsing is to get the image URL that fits the size of the mobile phone, and will not download the original image size.
