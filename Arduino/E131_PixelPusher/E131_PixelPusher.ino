#ifdef ESP32
#include <WiFi.h>
#include <AsyncUDP.h>
#include <AsyncTCP.h>            //https://github.com/me-no-dev/AsyncTCP
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <Update.h>
#include <DNSServer.h>
#elif defined(ESP8266)
#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>         //https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncUDP.h>         //https://github.com/me-no-dev/ESPAsyncUDP
#if defined(ESP8266) and (defined(PIO_PLATFORM) or defined(USE_EADNS))
#include <ESPAsyncDNSServer.h>   //https://github.com/devyte/ESPAsyncDNSServer
#else
#include <DNSServer.h>
#endif
#else
#error Platform not supported
#endif
#include <ESPAsyncE131.h>        //https://github.com/forkineye/ESPAsyncE131
#include <ESPAsyncWiFiManager.h> //https://github.com/alanswx/ESPAsyncWiFiManager
#include <ESPAsyncWebServer.h>   //https://github.com/me-no-dev/ESPAsyncWebServer
#include <NeoPixelBus.h>         //https://github.com/Makuna/NeoPixelBus
#include <EEPROM.h>
#include "version.h"

#define WIFI_HTM_GZ_PROGMEM   //comment to serve minimized html instead of gziped version 
//#define SERIAL_DEBUG        //uncomment to see if E1.31 data is received
//#define SHOW_FPS_SERIAL     //uncomment to see Serial FPS

#define HOSTNAME "E131PixelPusher"
#define HTTP_PORT 80

uint8_t START_UNIVERSE = 1;                  // First DMX Universe to listen for
uint8_t UNIVERSE_COUNT = 7;                  // Total number of Universes to listen for, starting at START_UNIVERSE max 7 for multicast and 12 for unicast
uint16_t ledCount = 12 * 170;                // 170 LEDs per Universe
bool unicast_flag = false;

#ifndef WIFI_HTM_GZ_PROGMEM
//size: 3202
static const char index_htm[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en"><head><meta http-equiv="Content-Type" content="text/html; charset=utf-8"><meta name="viewport" content="width=device-width"><script>function LoadBody(){var e="/data";fetch(e,{method:"GET"}).then(function(e){if(e.ok)return e.json();throw new Error("Network response was not ok.")}).then(function(e){console.log("Success:",JSON.stringify(e)),document.getElementById("pixelct").innerHTML=170*(e.uct-e.su+1),document.getElementById("mode").value=e.mode,document.getElementById("su").value=e.su,document.getElementById("uct").value=e.uct}).catch(function(e){console.error("Error:",e)})}function validateVal(){var e=document.getElementById("su").value,t=document.getElementById("uct").value,n=1190;return"unicast"==document.frmmr.mode.value&&(n=2040),!(e>t||t-e+1<=0||170*(t-e+1)>n)||(e>t&&alert("Starting universe cant be higher than total number of universes!"),t-e+1<=0&&alert("Number of Pixels cant be negative/zero!"),170*(t-e+1)>n&&alert("Cant access that many universes!"),!1)}function calcPixels(){document.getElementById("pixelct").innerHTML=170*(document.getElementById("uct").value-document.getElementById("su").value+1)}function checkUnicast(){"unicast"==document.frmmr.mode.value?(document.getElementById("su").max="12",document.getElementById("uct").max="12"):(document.getElementById("su").max="7",document.getElementById("uct").max="7")}</script><style>.subbt,body{text-align:center;color:#fff}.subbt,.title,a,body{color:#fff}.github a,.subbt,a{text-decoration:none}body{width:100%;height:100%;margin:auto;background-color:#c6b3d4;font-family:sans-serif;-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}#wrapper{width:250px;height:270px;border:2px solid #8531C6;border-radius:8px;margin:25px auto auto;background-color:#580797}.subbt{margin:20px;background-color:#008CBA;border:none;padding:10px 30px;display:inline-block;font-size:20px;border-radius:8px}.title{line-height:24px;display:block;padding:2px 0}.form{width:100%;padding:10px;margin:8px 0;box-sizing:border-box}.github{margin-top:10px}.github a{color:#EF5989;font-weight:700;font-size:16px}</style></head><body onload="LoadBody()"><div id="wrapper"><h4 class="title">E1.31 Pixel Pusher</h4><form name="frmmr" class="form" method="POST" action="/updateparams" onsubmit="return validateVal()">Pixels: <span id="pixelct"></span><br>Mode:<select id="mode" name="mode" onblur="checkUnicast()"><option value="unicast">UNICAST</option><option value="mulicast" selected>MULTICAST</option></select><br>Start Universe: <input id="su" name="su" type="number" placeholder="starting universe" max="12" min="1" onkeyup="this.value=this.value.replace(/[^\d]/,&#34;&#34;)" onblur="calcPixels()" onclick="calcPixels()"><br>Max Universe : <input id="uct" name="uct" type="number" placeholder="max universe" max="12" min="1" onkeyup="this.value=this.value.replace(/[^\d]/,&#34;&#34;)" onblur="calcPixels()" onclick="calcPixels()"><br><input class="subbt" type="submit" value="Update"></form></div><br><a href="/update">Update Firmware?</a><br><div class="github"><a href="https://github.com/debsahu/E131_PixelPusher">E131_PixelPusher by @debsahu</a></div><script></script></body></html>
)=====";
#else
//gziped: 1383 (~57% compression)
#define index_htm_gz_len 1383
static const uint8_t index_htm_gz[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x00, 0x0d, 0x95, 0x46, 0x5c, 0x00, 0x03, 0xcd, 0x57,
  0x6d, 0x53, 0xe3, 0x36, 0x10, 0xfe, 0x2b, 0x42, 0x4c, 0x99, 0xa4, 0x17,
  0xdb, 0x31, 0x2f, 0x07, 0x38, 0x71, 0xda, 0x3b, 0xca, 0xb5, 0xd7, 0xb9,
  0xe3, 0x98, 0x39, 0xe8, 0x4c, 0xa7, 0x6f, 0x23, 0xdb, 0xeb, 0x58, 0x8d,
  0x2d, 0xb9, 0x92, 0x4c, 0xc8, 0x01, 0xff, 0xbd, 0x2b, 0xcb, 0x0e, 0x81,
  0x1e, 0x57, 0xfa, 0xad, 0x5f, 0x88, 0x25, 0xed, 0xdb, 0xb3, 0xbb, 0x7a,
  0x56, 0x4c, 0xb7, 0xbe, 0xfb, 0x70, 0x72, 0xf1, 0xf3, 0xf9, 0x29, 0x29,
  0x4c, 0x55, 0xce, 0xa6, 0xf6, 0x2f, 0x29, 0x99, 0x98, 0xc7, 0x14, 0x04,
  0xc5, 0x35, 0xb0, 0x6c, 0x36, 0xad, 0xc0, 0x30, 0x14, 0x30, 0xb5, 0x07,
  0x7f, 0x35, 0xfc, 0x2a, 0xa6, 0x27, 0x52, 0x18, 0x10, 0xc6, 0xbb, 0x58,
  0xd5, 0x40, 0x49, 0xea, 0x56, 0x31, 0x35, 0x70, 0x6d, 0x02, 0x6b, 0x62,
  0x42, 0xd2, 0x82, 0x29, 0x0d, 0x26, 0x6e, 0x4c, 0xee, 0x1d, 0xd1, 0xce,
  0x84, 0x60, 0x15, 0xc4, 0xf4, 0x8a, 0xc3, 0xb2, 0x96, 0xca, 0x6c, 0x28,
  0x2e, 0x79, 0x66, 0x8a, 0x38, 0x83, 0x2b, 0x9e, 0x82, 0xd7, 0x2e, 0x50,
  0x45, 0xa7, 0x8a, 0xd7, 0x66, 0x96, 0x37, 0x22, 0x35, 0x5c, 0x0a, 0xf2,
  0x4e, 0xb2, 0xec, 0xb5, 0xcc, 0x56, 0x83, 0xe1, 0xcd, 0x15, 0x53, 0x04,
  0x2d, 0x05, 0x19, 0x33, 0x8c, 0x4e, 0x72, 0x30, 0x69, 0x31, 0x80, 0xd1,
  0x0d, 0x3a, 0x29, 0x64, 0x16, 0xd1, 0xef, 0x4f, 0x2f, 0xe8, 0xdd, 0xd0,
  0x37, 0x05, 0x88, 0x41, 0xaf, 0x3e, 0x80, 0xe1, 0x0d, 0xcf, 0x07, 0xe0,
  0xcb, 0xc5, 0x50, 0x81, 0x69, 0x94, 0x20, 0xe0, 0xff, 0xa9, 0xf1, 0x60,
  0x38, 0x31, 0x85, 0x92, 0x4b, 0x22, 0x60, 0x49, 0x4e, 0x95, 0x92, 0x6a,
  0x40, 0xcf, 0xc0, 0x2c, 0xa5, 0x5a, 0x10, 0x05, 0xba, 0x96, 0x42, 0x03,
  0x59, 0x32, 0x4d, 0x84, 0x34, 0x44, 0x2e, 0x7c, 0x3a, 0xfc, 0x9c, 0x69,
  0x84, 0xa2, 0x65, 0x09, 0x7e, 0x29, 0xe7, 0x03, 0xfa, 0xb1, 0x49, 0x53,
  0xd0, 0x3a, 0xa2, 0xa3, 0x1f, 0x3f, 0x7e, 0x38, 0xf3, 0xb5, 0x51, 0x5c,
  0xcc, 0x79, 0xbe, 0x42, 0xc1, 0xe1, 0x28, 0x93, 0x69, 0x53, 0x21, 0x6a,
  0x7f, 0x0e, 0xe6, 0xb4, 0x04, 0xfb, 0xf9, 0x7a, 0xf5, 0x36, 0x1b, 0xd0,
  0x9a, 0x5f, 0x43, 0x99, 0x1a, 0x3a, 0xf4, 0xb9, 0x10, 0xa0, 0x7e, 0xb8,
  0x78, 0xff, 0x2e, 0x0e, 0x0f, 0xc7, 0x5f, 0x63, 0xcc, 0x4d, 0x6a, 0x3c,
  0xf0, 0x75, 0xf3, 0x22, 0xfc, 0x82, 0x7e, 0x25, 0x33, 0x40, 0xe5, 0x2b,
  0x56, 0x36, 0x10, 0x83, 0x6f, 0x97, 0x4f, 0x0b, 0xeb, 0x66, 0x43, 0x54,
  0x37, 0x4f, 0x0b, 0x36, 0x6d, 0x44, 0xbd, 0x24, 0xae, 0x10, 0x7d, 0xca,
  0x6c, 0xc2, 0x3f, 0x07, 0x1f, 0x5c, 0xfe, 0xda, 0x34, 0x22, 0x7c, 0xc0,
  0x5c, 0xdd, 0xad, 0x0b, 0x88, 0x46, 0x38, 0x56, 0x0c, 0x7e, 0x62, 0xe5,
  0xba, 0x86, 0xcf, 0x08, 0x70, 0x64, 0x9e, 0x96, 0xda, 0x88, 0x6e, 0x24,
  0xe2, 0x30, 0x3c, 0x1e, 0x4f, 0x5c, 0x71, 0x69, 0x23, 0x78, 0xca, 0xb4,
  0xa1, 0xf1, 0xbd, 0x72, 0xae, 0xaa, 0x4a, 0xb5, 0x79, 0x71, 0x1a, 0x3b,
  0x3b, 0x03, 0x11, 0xef, 0x8e, 0xf7, 0xc7, 0xc3, 0xd1, 0xd6, 0x00, 0x66,
  0xe6, 0xf6, 0x16, 0xb3, 0xfc, 0x22, 0x9c, 0xc6, 0xe3, 0xdb, 0xdb, 0x36,
  0xf1, 0xed, 0x72, 0x38, 0x13, 0xc3, 0xdb, 0x5b, 0x7b, 0xbe, 0xb3, 0xc3,
  0x4a, 0x50, 0x06, 0xeb, 0x6b, 0x98, 0x32, 0x58, 0x52, 0x82, 0x4e, 0xae,
  0x00, 0xfb, 0x9c, 0xa4, 0x4c, 0x18, 0x92, 0x00, 0x29, 0xf8, 0xbc, 0x00,
  0x45, 0x4c, 0xc1, 0x04, 0x31, 0xd2, 0xb0, 0x92, 0x88, 0xa6, 0x4a, 0x70,
  0x47, 0xe6, 0x6b, 0x61, 0xbd, 0x45, 0x87, 0xa3, 0xde, 0xd3, 0xda, 0xe6,
  0xd9, 0x5a, 0xee, 0xdc, 0xf6, 0x81, 0x5e, 0x9b, 0x14, 0x30, 0x67, 0x06,
  0x35, 0x83, 0x4f, 0xa0, 0xa4, 0x55, 0x7d, 0x10, 0xda, 0x5a, 0xff, 0xc4,
  0x8a, 0xb3, 0xb6, 0xef, 0xac, 0x7b, 0x43, 0x2a, 0x26, 0x56, 0x0f, 0x7d,
  0x6e, 0x85, 0x1b, 0xc5, 0x48, 0x59, 0x99, 0x3a, 0x4f, 0x58, 0x8b, 0xff,
  0xde, 0x93, 0xcf, 0xa9, 0x88, 0xf7, 0x8c, 0xe2, 0xbe, 0x78, 0x10, 0x53,
  0x01, 0xe9, 0xe2, 0xd2, 0x15, 0x0e, 0xa3, 0x7a, 0x4e, 0x0d, 0xbf, 0x79,
  0x3a, 0x92, 0xd6, 0x49, 0xc5, 0xae, 0x63, 0x1a, 0xee, 0xd2, 0x7f, 0x6b,
  0xf0, 0x5e, 0x6e, 0x18, 0x3d, 0xc7, 0xe0, 0xe1, 0xf3, 0xec, 0x1d, 0x22,
  0x57, 0x4c, 0x83, 0x8e, 0xc7, 0xa6, 0xda, 0xac, 0x4a, 0x98, 0xe1, 0x65,
  0x4b, 0x12, 0x33, 0x4a, 0x90, 0xc8, 0x6e, 0x2c, 0x5f, 0x7a, 0x78, 0x29,
  0xe6, 0x22, 0x4a, 0x51, 0x1f, 0xd4, 0x24, 0x95, 0x25, 0x5e, 0x9c, 0xed,
  0x3c, 0xcf, 0xef, 0x3a, 0x41, 0xdf, 0x70, 0x53, 0xc2, 0x88, 0x39, 0x8d,
  0xcd, 0xf3, 0x39, 0x37, 0x45, 0x93, 0x10, 0x36, 0xea, 0x24, 0x99, 0xb3,
  0x97, 0x41, 0x2a, 0x15, 0xb3, 0xf9, 0x8c, 0x84, 0x14, 0x70, 0xd7, 0xea,
  0xb5, 0x9c, 0x1a, 0x85, 0xe3, 0xf1, 0x57, 0x93, 0x02, 0xb0, 0x47, 0x8d,
  0xfb, 0xae, 0x98, 0x9a, 0x73, 0x11, 0xb1, 0xc6, 0xc8, 0x49, 0xc2, 0xd2,
  0xc5, 0x5c, 0xc9, 0x46, 0x64, 0x5e, 0xe7, 0x25, 0x7d, 0x99, 0xec, 0x65,
  0xfb, 0x93, 0x1c, 0x59, 0xda, 0xcb, 0x59, 0xc5, 0xcb, 0x55, 0xa4, 0x99,
  0xd0, 0x9e, 0x06, 0xc5, 0xf3, 0x89, 0xb7, 0x84, 0x64, 0xc1, 0x8d, 0x97,
  0xc8, 0x6b, 0x4f, 0xf3, 0x4f, 0x78, 0x1d, 0xa2, 0x44, 0xaa, 0x0c, 0x94,
  0xdd, 0x99, 0x78, 0x95, 0xfc, 0xf4, 0xc4, 0xd1, 0x67, 0x77, 0xef, 0xb6,
  0x97, 0x8a, 0xd5, 0x35, 0xa8, 0x2e, 0xd4, 0xdd, 0x83, 0x71, 0x7d, 0xdd,
  0xc7, 0xba, 0x7b, 0x68, 0x17, 0x4e, 0x38, 0xda, 0xad, 0xaf, 0x09, 0x72,
  0x0d, 0xcf, 0xc8, 0xf6, 0xd1, 0xc1, 0x5e, 0x78, 0xf2, 0xb2, 0x3b, 0xf0,
  0x14, 0xcb, 0x78, 0xa3, 0xa3, 0x23, 0x14, 0xed, 0x70, 0xed, 0x1e, 0xa0,
  0xac, 0x05, 0x47, 0x9e, 0x40, 0x78, 0x70, 0x34, 0x3e, 0x3c, 0x3e, 0xec,
  0x52, 0x7d, 0xd3, 0x6b, 0xb5, 0xce, 0xfe, 0x21, 0x3b, 0x1e, 0x1f, 0x9d,
  0xbc, 0x7e, 0xd5, 0x47, 0x61, 0x53, 0x3b, 0xa9, 0x59, 0x96, 0x59, 0x18,
  0x21, 0x6a, 0x90, 0x3d, 0xab, 0x96, 0x71, 0x5d, 0x97, 0x6c, 0x15, 0x71,
  0x51, 0x72, 0x01, 0x5e, 0x52, 0xca, 0x74, 0xe1, 0x12, 0x88, 0x88, 0xa1,
  0x33, 0xfd, 0x38, 0xdc, 0x3b, 0x57, 0xe3, 0x9b, 0x56, 0xa5, 0x87, 0xbc,
  0xbf, 0x61, 0xcd, 0x99, 0xe9, 0xbd, 0xd9, 0x04, 0x8c, 0xef, 0xfc, 0x5c,
  0xaa, 0x6a, 0xb3, 0xae, 0x9b, 0xc1, 0xf4, 0x09, 0x38, 0xb2, 0xa2, 0x4f,
  0x24, 0xbc, 0xeb, 0x9f, 0x0e, 0xb5, 0x67, 0x64, 0xdd, 0xaa, 0xde, 0xf7,
  0x55, 0xdf, 0x6c, 0xa7, 0x6f, 0x0e, 0x8e, 0x8f, 0x8e, 0x1d, 0x8a, 0xa5,
  0x8b, 0xee, 0x70, 0x3c, 0xde, 0x40, 0x15, 0xbe, 0x44, 0x35, 0xec, 0xf3,
  0xb6, 0xbf, 0xa7, 0x81, 0x7b, 0x33, 0xd8, 0xb6, 0x23, 0x52, 0x94, 0x38,
  0xb4, 0x63, 0x7a, 0x3f, 0xba, 0x71, 0xae, 0x67, 0xfc, 0x8a, 0x70, 0xdc,
  0xec, 0x0a, 0x6e, 0x5f, 0x19, 0xfb, 0x24, 0x2d, 0x99, 0xd6, 0xf8, 0x82,
  0xb0, 0x89, 0xa0, 0xb3, 0xd3, 0xd0, 0xdf, 0x0b, 0x1d, 0x11, 0x92, 0xf3,
  0x46, 0x23, 0xa5, 0xa2, 0xd9, 0xfd, 0xd9, 0xd4, 0x62, 0xee, 0x5e, 0x11,
  0x2d, 0x11, 0xd0, 0x5e, 0xcf, 0x1e, 0x50, 0xe2, 0xe6, 0x7f, 0x4c, 0xcf,
  0x3f, 0x7c, 0xbc, 0xa0, 0x48, 0x86, 0xf6, 0x12, 0xe0, 0x2b, 0xa1, 0xa9,
  0xed, 0xd4, 0xa9, 0x99, 0x62, 0x95, 0xa6, 0x18, 0x13, 0x56, 0xbb, 0xe2,
  0xf8, 0xea, 0xe8, 0x9e, 0x01, 0x0f, 0xc6, 0x12, 0x9d, 0x39, 0x52, 0x8c,
  0xc8, 0x54, 0xd7, 0x48, 0xe2, 0x36, 0xd0, 0x9e, 0x04, 0x11, 0x9b, 0xdd,
  0x43, 0x6c, 0x6a, 0xf6, 0x1e, 0x19, 0x28, 0x9a, 0x6a, 0x28, 0x21, 0x35,
  0xad, 0x50, 0x3b, 0x7d, 0xbb, 0xd8, 0xdc, 0xb7, 0x14, 0x49, 0xd9, 0xa8,
  0x98, 0x3e, 0x64, 0x35, 0xb4, 0x22, 0xeb, 0x7e, 0x1c, 0xe2, 0x4c, 0x5d,
  0x93, 0xdc, 0xec, 0xf2, 0xec, 0xed, 0xc9, 0xab, 0x8f, 0x17, 0xd3, 0xc0,
  0x9d, 0x3f, 0x96, 0xab, 0x9a, 0xd2, 0x09, 0x12, 0xe7, 0x15, 0xb2, 0xd9,
  0xfb, 0xcb, 0x77, 0x17, 0x8f, 0x74, 0x02, 0x77, 0xd8, 0xc6, 0xd8, 0x8e,
  0x29, 0x72, 0xd9, 0x8d, 0x00, 0x44, 0xc4, 0x45, 0xdd, 0xb8, 0x68, 0x91,
  0xca, 0xba, 0x58, 0xed, 0x97, 0xc1, 0x67, 0x5c, 0x4c, 0xdd, 0xa4, 0xa2,
  0x04, 0x5b, 0x2e, 0x85, 0x42, 0x96, 0xd8, 0x28, 0x78, 0xfc, 0x78, 0xd4,
  0x61, 0x96, 0x3b, 0xaa, 0x24, 0x15, 0xc7, 0xec, 0x86, 0x16, 0xe8, 0x02,
  0x56, 0x4d, 0x8d, 0xd5, 0x2b, 0xb8, 0xee, 0x9e, 0x0a, 0xf7, 0x9f, 0xbe,
  0x82, 0xd6, 0xe2, 0x20, 0xf8, 0xe5, 0xf7, 0x5f, 0xb3, 0xdf, 0x82, 0xd1,
  0xce, 0xf6, 0xde, 0xfe, 0xa4, 0xfd, 0x33, 0xdc, 0x48, 0xd2, 0xc6, 0x38,
  0xb2, 0xbb, 0x29, 0x82, 0x5d, 0x3c, 0xda, 0x76, 0x89, 0x67, 0xd7, 0x6b,
  0x48, 0xe4, 0x01, 0x26, 0x4b, 0xbc, 0x1d, 0xa8, 0xf6, 0xf3, 0x0b, 0xa8,
  0x10, 0xc2, 0xff, 0x06, 0x50, 0x87, 0xa0, 0x6b, 0xe4, 0x96, 0x89, 0xfa,
  0xd8, 0x5d, 0xa3, 0xd2, 0xbe, 0x03, 0x2e, 0xdb, 0x3e, 0xb6, 0x6d, 0x68,
  0xdb, 0x1d, 0x7f, 0xf0, 0x26, 0x39, 0x13, 0xf8, 0x3a, 0x57, 0x90, 0xaf,
  0x5b, 0x1d, 0x7b, 0xa9, 0xfd, 0x25, 0x6f, 0xb8, 0xaa, 0x96, 0x4c, 0xc1,
  0x37, 0xd3, 0x80, 0x39, 0x49, 0x7b, 0xf9, 0x3a, 0x57, 0xee, 0x9e, 0xd3,
  0x7b, 0x6d, 0xfb, 0xc2, 0xd7, 0x51, 0x10, 0xb8, 0x03, 0x3f, 0x95, 0x55,
  0x90, 0x41, 0xa2, 0x59, 0xd1, 0x04, 0xa7, 0xe1, 0x5e, 0xf8, 0x47, 0x1b,
  0xb7, 0xbb, 0x8e, 0xf6, 0x8a, 0x3e, 0xdc, 0x21, 0xc9, 0x8a, 0x7c, 0xdb,
  0x89, 0xb7, 0xde, 0x5c, 0x74, 0xfd, 0xf0, 0x5b, 0x4f, 0xc1, 0xc0, 0x12,
  0x83, 0x65, 0x09, 0xfb, 0xff, 0xc6, 0xdf, 0x38, 0x71, 0xd5, 0x73, 0x7f,
  0x0c, 0x00, 0x00
};
#endif

//ESPAsyncE131 pointer
ESPAsyncE131* e131;
AsyncWebServer server(HTTP_PORT);

#ifdef ESP32
  //#define PIN 2 //Use any pin under 32
  //NeoEsp32BitBangWs2813Method dma = NeoEsp32BitBangWs2813Method(PIN, ledCount, 3);
  
  //APA102/DotStar
  //Hardware SPI method: GPIO18 is CLK, GPIO23 is DATA
  DotStarSpiMethod dma = DotStarSpiMethod(ledCount, 3); 
  //
  //Software SPI method: Any pin can be clock and data
  //#define PIN_CLK 18
  //#define PIN_DATA 23
  //DotStarMethod dma = DotStarMethod(PIN_CLK, PIN_DATA, ledCount, 3);
  
#elif defined(ESP8266)
  NeoEsp8266Dma800KbpsMethod dma = NeoEsp8266Dma800KbpsMethod(ledCount, 3);                     //uses RX/GPIO3 pin

  //APA102/DotStar
  //Hardware SPI method: GPIO14 is CLK, GPIO13 is DATA
  //DotStarSpiMethod dma = DotStarSpiMethod(ledCount, 3); 
  //
  //Software SPI method: Any pin can be clock and data
  //#define PIN_CLK 14
  //#define PIN_DATA 13
  //DotStarMethod dma = DotStarMethod(PIN_CLK, PIN_DATA, ledCount, 3);
#endif

#if defined(ESP8266) and (defined(PIO_PLATFORM) or defined(USE_EADNS))
AsyncDNSServer dns;
#else
DNSServer dns;
#endif

struct {
    bool unicast = false;
    uint16_t startUniverse = START_UNIVERSE;
    uint16_t noOfUniverses = UNIVERSE_COUNT;
} config;

int eeprom_addr = 0;

uint8_t *pixel = (uint8_t *)malloc(dma.getPixelsSize());

#ifdef SHOW_FPS_SERIAL
uint64_t frameCt = 0;
uint64_t PM = 0;
float interval = 10 * 1000.0; // 10s
#endif

bool shouldReboot = false;
const char update_html[] PROGMEM = "<!DOCTYPE html><html lang=\"en\"><head><title>Firmware Update</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><meta name=\"viewport\" content=\"width=device-width\"><link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"favicon.ico\"></head><body><h3>Update Firmware</h3><br><form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"update\"> <input type=\"submit\" value=\"Update\"></form></body></html>";

void initE131(void){
    uint8_t TOTAL_UNIVERSES = (UNIVERSE_COUNT - START_UNIVERSE + 1);
    ledCount = TOTAL_UNIVERSES * 170;
    e131 = new ESPAsyncE131(TOTAL_UNIVERSES);
    if (e131->begin((unicast_flag) ? E131_UNICAST : E131_MULTICAST, START_UNIVERSE, UNIVERSE_COUNT)) // Listen via Unicast/Multicast
        Serial.println(F(">>> Listening for E1.31 data..."));
    else
        Serial.println(F(">>> e131.begin failed :("));
}

void readEEPROM(void){
    EEPROM.get(eeprom_addr, config);
    unicast_flag = config.unicast;
    START_UNIVERSE = config.startUniverse;
    UNIVERSE_COUNT = config.noOfUniverses;
    #ifdef SERIAL_DEBUG
    Serial.printf("READ>> Mode: %s Start Universe: %d No of Universes: %d", (unicast_flag)?"unicast":"multicast", START_UNIVERSE, UNIVERSE_COUNT);
    #endif
}

void writeEEPROM(void){
    config.unicast = unicast_flag;
    config.startUniverse = START_UNIVERSE;
    config.noOfUniverses = UNIVERSE_COUNT;
    #ifdef SERIAL_DEBUG
    Serial.printf("WRITE>> Mode: %s Start Universe: %d No of Universes: %d", (unicast_flag)?"unicast":"multicast", START_UNIVERSE, UNIVERSE_COUNT);
    #endif
    EEPROM.put(eeprom_addr, config);
    EEPROM.commit();
}

void setup()
{
    SPIFFS.begin();
    EEPROM.begin(512);
    Serial.begin(115200);
    delay(10);

    Serial.println();
    readEEPROM();

    char NameChipId[64] = {0}, chipId[9] = {0};
    #ifdef ESP32
        Serial.print("Hardware SPI //DATA PIN: ");
        Serial.println(MOSI); //GPIO23?
        Serial.print("Hardware SPI //CLOCK PIN: ");
        Serial.println(SCK);  //GPIO18?
        snprintf(chipId, sizeof(chipId), "%08x", (uint32_t)ESP.getEfuseMac());
        snprintf(NameChipId, sizeof(NameChipId), "%s_%08x", HOSTNAME, (uint32_t)ESP.getEfuseMac());

        WiFi.mode(WIFI_STA); // Make sure you're in station mode
        WiFi.setHostname(const_cast<char *>(NameChipId));
        AsyncWiFiManager wifiManager(&server, &dns);
    #else
        snprintf(chipId, sizeof(chipId), "%06x", ESP.getChipId());
        snprintf(NameChipId, sizeof(NameChipId), "%s_%06x", HOSTNAME, ESP.getChipId());

        WiFi.mode(WIFI_STA); // Make sure you're in station mode
        WiFi.hostname(const_cast<char *>(NameChipId));
        AsyncWiFiManager wifiManager(&server, &dns); //Local intialization. Once its business is done, there is no need to keep it around
    #endif
    wifiManager.setConfigPortalTimeout(180);     //sets timeout until configuration portal gets turned off, useful to make it all retry or go to sleep in seconds
    if (!wifiManager.autoConnect(NameChipId))
    {
        Serial.println("Failed to connect and hit timeout");
        ESP.restart();
    }
    Serial.println("");
    Serial.print(F(">>> Connected with IP: "));
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        #ifdef WIFI_HTM_GZ_PROGMEM
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_htm_gz, index_htm_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        #else
        request->send_P(200, "text/html", index_htm);
        #endif
    });
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"ct\":" + String(ledCount) + ",\"mode\":\"" + String((unicast_flag) ? "unicast" : "multicast") + "\",\"su\":" + String(START_UNIVERSE) + ",\"uct\":" + String(UNIVERSE_COUNT) + "}");
    });
    server.on("/updateparams", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool hasReqArgs = false;
        if (request->hasParam("pixelct", true)) {
            unicast_flag = (request->getParam("mode", true)->value() == "unicast") ? true : false;
            hasReqArgs = true;
        }
        if (request->hasParam("su", true)) {
            START_UNIVERSE = constrain(request->getParam("su", true)->value().toInt(), 1, (unicast_flag)?12:7);
            hasReqArgs = true;
        }
        if (request->hasParam("uct", true)) {
            UNIVERSE_COUNT = constrain(request->getParam("uct", true)->value().toInt(), 1, (unicast_flag)?12:7);
            hasReqArgs = true;
        }
        if (hasReqArgs){
            initE131();
            writeEEPROM();
            hasReqArgs = false;
        }
        request->send(200, "text/html", "<META http-equiv='refresh' content='3;URL=/'><body align=center>LED Count: "+ String(ledCount) + ", Mode: " + String((unicast_flag) ? "unicast" : "multicast") + ", Starting Universe: " + String(START_UNIVERSE) + ", Universe Count: " + String(UNIVERSE_COUNT) + "</body>");
    });
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", SKETCH_VERSION);
    });
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", update_html);
        request->send(response);
    });
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", shouldReboot ? "<META http-equiv='refresh' content='15;URL=/'>Update Success, rebooting..." : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
        }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
          if (!filename.endsWith(".bin")) {
                return;
            }
            if(!index){
                if(Serial) Serial.printf("Update Start: %s\n", filename.c_str());
                #ifdef ESP32
                uint32_t maxSketchSpace = len; // for ESP32 you just supply the length of file
                #elif defined(ESP8266)
                Update.runAsync(true); // There is no async for ESP32
                uint32_t maxSketchSpace = ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
                #endif
                if(!Update.begin(maxSketchSpace)) {
                if(Serial) Update.printError(Serial);
                }
            }
            if(!Update.hasError()){
                if(Update.write(data, len) != len) {
                if(Serial) Update.printError(Serial);
                }
            }
            if(final){
                if(Update.end(true)) 
                if(Serial) Serial.printf("Update Success: %uB\n", index+len);
                else {
                if(Serial) Update.printError(Serial);
                }
            } });

    MDNS.setInstanceName(String(HOSTNAME " (" + String(chipId) + ")").c_str());
    if (MDNS.begin(NameChipId)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        MDNS.addService("e131", "udp", E131_DEFAULT_PORT);
        #ifndef ARDUINO_ESP8266_RELEASE_2_4_2
        MDNS.addServiceTxt("e131", "udp", "CID", String(chipId));
        MDNS.addServiceTxt("e131", "udp", "Model", "E131_PixelPusher");
        MDNS.addServiceTxt("e131", "udp", "Manuf", "debsahu");
        #endif
        Serial.printf(">>> MDNS Started: http://%s.local/\n", NameChipId);
    } else {
        Serial.println(F(">>> Error setting up mDNS responder <<<"));
    }

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.begin();

    initE131();
    delay(1000);
#if !defined(SHOW_FPS_SERIAL) or !defined(SERIAL_DEBUG)
    Serial.end();
#endif
    dma.Initialize();
    memset(pixel, 0, sizeof(pixel));
}

void loop()
{
    #ifdef ESP8266
        MDNS.update();
    #endif

    if (!e131->isEmpty())
    {
        e131_packet_t packet;
        e131->pull(&packet); // Pull packet from ring buffer

        uint16_t universe = htons(packet.universe);
        uint8_t *data = packet.property_values + 1;

        if (universe < START_UNIVERSE || universe > UNIVERSE_COUNT)
            return; //async will take care about filling the buffer

        #ifdef SERIAL_DEBUG
        Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
                      htons(packet.universe),                 // The Universe for this packet
                      htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                      e131->stats.num_packets,                 // Packet counter
                      e131->stats.packet_errors,               // Packet error counter
                      packet.property_values[1]);             // Dimmer data for Channel 1
        #endif

        uint16_t multipacketOffset = (universe - START_UNIVERSE) * 170; //if more than 170 LEDs (510 channels), client will send in next higher universe
        if (ledCount <= multipacketOffset)
            return;
        uint16_t len = (170 + multipacketOffset > ledCount) ? (ledCount - multipacketOffset) * 3 : 510;
        memcpy(pixel + multipacketOffset * 3, data, len); // Burden on source to send in correct color order
    }

    if (dma.IsReadyToUpdate())
    {
        memcpy(dma.getPixels(), pixel, dma.getPixelsSize());
        dma.Update();
#ifdef SHOW_FPS_SERIAL
        frameCt++;
#endif
    }

#ifdef SHOW_FPS_SERIAL
    if (millis() - PM >= interval)
    {
        PM = millis();
        Serial.printf("FPS: %.2f\n", interval / frameCt);
        frameCt = 0;
    }
#endif

    if(shouldReboot) {
        #ifdef SHOW_FPS_SERIAL
        Serial.println("Rebooting...");
        #endif
        delay(100);
        ESP.restart();
    }
}