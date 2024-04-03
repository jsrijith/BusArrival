# BusArrival
More information related to this project can be found at 
http://www.srisoftstudios.com/arduino/catchingthebusfromthecouch

Hurray!!. This is my first Arduino project and would like to share with all makers out there. I am just getting started in this wonderful world of electronics and circuitry. After playing with few example projects  that was part of the arduino starter kit, I wanted to do something useful which I can use everyday. The idea of having always-on display with bus arrival information seemed as a good starting point to take my learning to the next level. Lets get into the ABC of this :)
API
LTA publishes variety of transport related data as rest API's for public and community use. Just quickly created an account received the API keys, tested few request from using RequestMaker.. Bingo the data is ready!!
Bill Of Materials
Now lets do some shopping. This little device will be in my living room connected to my home WiFi so need a arduino compatible WiFi board and a decent LCD.  After some quick search in google I narrowed down to the following
Arduino Wifi Shield
ESP8266 Boards
ESP8266 was my pick as it was super cheap and I immediately ordered few from Ali Express. While waiting for the board to arrive (Usually Ali Express orders take 20~30days turnaround time for free shipping) found that there are many variants of ESP8266 and the one I ordered was not a good choice. Head to this post to know more on ESP8266 variants.
The next hardware is LCD display. My arduino starter kit came with a 16 * 2 display. For this project I needed a little bigger content display. So ordered a 20 * 4 LCD display with I2C interface which is just 4 wire hook-up which reduces lots of wire clutters.
