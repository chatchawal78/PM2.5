#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>
#define BUZZER_RELAY_PIN 26
#define PM25_DANGER_ON 50
#define PM25_DANGER_OFF 45
#include <ESP32Servo.h>
// ===== SERVO =====
Servo servo1;
Servo servo2;
#define SERVO1_PIN 12
#define SERVO2_PIN 13
bool pmReady = false;
bool sendingScreen = false;
bool servoIsUp = false;  
unsigned long startTime;
// ================= BUZZER STATE =================
bool buzzerActive = false;
bool buzzerState = false;
bool dangerLineSent = false;
unsigned long lastBuzzerToggle = 0;
int beepCount = 0;        
const int maxBeeps = 10;  
const unsigned long beepOnTime = 200;   
const unsigned long beepOffTime = 300;  
// ================= STATE =================
bool dangerMode = false;
bool hasPMData = false;
const unsigned long LINE_INTERVAL = 5UL * 60UL * 1000UL;  
// ================= DANGER =================
// unsigned long lastDangerNotify = 0;
const unsigned long DANGER_INTERVAL = 5UL * 60UL * 1000UL;  
// ================= INTERVALS =================
const unsigned long TFT_INTERVAL = 1000;  
// const unsigned long LINE_INTERVAL   = 300000;  
const unsigned long SHEET_INTERVAL = 120000;  
// ================= TIMERS =================
unsigned long lastTFTUpdate = 0;
unsigned long lastLineNotify = 0;
unsigned long lastSheetSend = 0;
unsigned long lastWifiCheck = 0;
// ================= SENSOR VALUES =================
float humidity = 0.0;
float temperature = 0.0;
double latitude = 0.0;
double longitude = 0.0;
// ================= PM =================
int pm1_0 = 0;
int pm2_5 = 0;
int pm10 = 0;
int aqi = 0;
#include "epd_bitmap_allArray.h"
//===========gps=================
#include <TinyGPSPlus.h>
// ===============================
//ฟังก์ชันวัน / เวลา 
String getDateTH() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char buf[30];
  int yearTH = timeinfo.tm_year + 1900 + 543;
  sprintf(buf, "%02d/%02d/%d",
          timeinfo.tm_mday,
          timeinfo.tm_mon + 1,
          yearTH);
  return String(buf);
}
String getTimeTH() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char buf[10];
  sprintf(buf, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  return String(buf);
}
String getDateISO() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "";
  }
  char buf[20];
  sprintf(buf, "%04d-%02d-%02d",
          timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday);
  return String(buf);
}
// ===============================
// 🔔 ฟังก์ชันสร้าง "ข้อความสั้น" สำหรับ LINE
// ===============================
String buildShortMessage() {
  if (aqi > 150) {
    return "---------------------------------------------------\n"
           "📟 AIR QUALITY REPORT 🌸\n"
           "---------------------------------------------------\n"
           "• 📆 รายงานวันที่ "
           + getDateTH() + "\n"
                           "----------->เวลา : "
           + getTimeTH() + " น.<-----------\n"
                           "🔴 ค่า AQI : "
           + String(aqi) + " (มีผลกระทบต่อสุขภาพ)\n"
                           "สภาพอากาศอยู่ในระดับเฝ้าระวัง ควรหลีกเลี่ยงกิจกรรมกลางแจ้ง\n"
                           "- 🚨 PM2.5 : "
           + String(pm2_5) + " µg/m³ \n"
                             "--------------------------------------------------\n"
                             "🫂 ดูแลสุขภาพกันด้วยนะครับ🚨\n"
                             "---------------------------------------------------\n"
                             "ⓘ กดปุ่มที่ชิ้นงานค้าง 5-10 วินาที เพื่อดูรายละเอียดข้อมูลเพิ่มเติม";
  }

  else if (aqi > 100) {
    return "---------------------------------------------------\n"
           "📟 AIR QUALITY REPORT 🌸\n"
           "---------------------------------------------------\n"
           "• 📆 รายงานวันที่ "
           + getDateTH() + "\n"
                           "----------->เวลา : "
           + getTimeTH() + " น.<-----------\n"
                           "🟠 ค่า AQI : "
           + String(aqi) + " (เริ่มมีผลต่อสุขภาพ)\n"
                           "สภาพอากาศอยู่ในเกณฑ์ที่เริ่มไม่ปลอดภัยควรใส่หน้ากากเมื่อออกจากบ้าน\n"
                           "- 😷 PM2.5 : "
           + String(pm2_5) + " µg/m³ \n"
                             "--------------------------------------------------\n"
                             "🥰 ดูแลสุขภาพกันด้วยนะครับ😊\n"
                             "---------------------------------------------------\n"
                             "ⓘ กดปุ่มที่ชิ้นงานค้าง 5-10 วินาที เพื่อดูรายละเอียดข้อมูลเพิ่มเติม";
  }

  else if (aqi > 50) {
    return "---------------------------------------------------\n"
           "📟 AIR QUALITY REPORT 🌸\n"
           "---------------------------------------------------\n"
           "• 📆 รายงานวันที่ "
           + getDateTH() + "\n"
                           "----------->เวลา : "
           + getTimeTH() + " น.<-----------\n"
                           "🟡 ค่า AQI : "
           + String(aqi) + " (ปานกลาง)\n"
                           "สภาพอากาศอยู่ในเกณฑ์ที่เริ่มมีผลต่อสุขภาพ ลดเวลาอยู่นอกอาคารหรือพื้นที่กลางแจ้ง\n"
                           "- 🤧 PM2.5 : "
           + String(pm2_5) + " µg/m³\n"
                             "--------------------------------------------------\n"
                             "🥰 ดูแลสุขภาพกันด้วยนะครับ😊\n"
                             "---------------------------------------------------\n"
                             "ⓘ กดปุ่มที่ชิ้นงานค้าง 5-10 วินาที เพื่อดูรายละเอียดข้อมูลเพิ่มเติม";
  } else {
    return "---------------------------------------------------\n"
           "📟 AIR QUALITY REPORT 🌸\n"
           "---------------------------------------------------\n"
           "• 📆 รายงานวันที่ "
           + getDateTH() + "\n"
                           "----------->เวลา : "
           + getTimeTH() + " น.<-----------\n"
                           "🟢 ค่า AQI : "
           + String(aqi) + " (ดี)\n"
                           "สภาพอากาศอยู่ในเกณฑ์ที่ดีมาก ใช้ชีวิตกลางแจ้งได้ตามปกติ\n"
                           "- 😊 PM2.5 : "
           + String(pm2_5) + " µg/m³ \n"
                             "--------------------------------------------------\n"
                             "🥰 ดูแลสุขภาพกันด้วยนะครับ😊\n"
                             "---------------------------------------------------\n"
                             "ⓘ กดปุ่มที่ชิ้นงานค้าง 5-10 วินาที เพื่อดูรายละเอียดข้อมูลเพิ่มเติม";
  }
}

///////////รายงานเต็ม
String buildWelcomeMessage() {
  return "                           🌱\n"
         " ระบบรายงานคุณภาพอากาศ PM\n\n"
         "👋🏻สวัสดีครับยินดีต้อนรับครับ ☺\n"
         "-------------------------------------------------\n"
         "- ⏳ ระบบกำลังเตรียมเซนเซอร์ กรุณารอสักครู่นะครับ \n"
         "- 📊 ระบบจะรายงานค่าฝุ่นทุก 5 นาที\n"
         "- 🔘 หรือกดปุ่มที่อุปกรณ์เพื่อดูค่าล่าสุดได้ทันที\n"
         "      -----------------------------------------\n"
         " •🔔 หมายเหตุ  :\n"
         "ข้อมูลค่าฝุ่นที่แสดงจะเป็นข้อมูลหลังจากเซนเซอร์มีความเสถียร\n\n"
         "- 📒 ดูข้อมูลย้อนหลังผ่านเว็บไซต์ Google Sheets :\n"
         "https://docs.google.com/spreadsheets/d/1VkdY-OSyzozqbb1S6-wXs01ro6ceTDRfbecOsr4gasA/edit?usp=sharing"
         "                      -----------";
}
String buildProjectInfoMessage() {
  return "👤 ผู้จัดทำ : นายชัชวาลย์ เมฆารักษ์กุล\n"
         "🏫 ชื่อโครงงาน : การพัฒนาระบบแจ้งเตือนวัดค่าฝุ่นละออง PM2.5 "
         "ผ่านแอพพลิเคชั่นไลน์โลเคชั่นแบบเรียลไทม์";
}
// ===============================
// 🔥 วางชุดข้อความรายงานเต็มทั้งหมดตรงนี้
String fullGreenReport() {
  aqi = calculateAQI(pm2_5);
  return "---------------------------------------------------\n"
         "📟 AIR QUALITY REPORT 🌸\n"
         "---------------------------------------------------\n"
         "• 📆 รายงานวันที่ "
         + getDateTH() + "\n"
                         "----------->เวลา : "
         + getTimeTH() + " น.<-----------\n"
                         "🟢 ค่า AQI : "
         + String(aqi) + "\n"
                         "สภาพอากาศ ณ ตอนนี้อยู่ในเกณฑ์ที่ดีมาก สามารถใช้ชีวิตได้ตามปกติเลยครับ😊🌿\n"
                         "---------------------------------------------------\n"
                         "• ✅ คำแนะนำ :\n"
                         "- ทำกิจกรรมกลางแจ้งปกติ\n"
                         "- เปิดประตู-หน้าต่างรับอากาศได้\n"
                         "- เหมาะกับการออกกำลังกายมาก\n"
                         "---------------------------------------------------\n"
                         "• 🌫 ค่าฝุ่นละออง ปัจจุบัน :\n"
                         "- 👻 PM1.0 : "
         + String(pm1_0) + " µg/m³\n"
                           "- 😊 PM2.5 : "
         + String(pm2_5) + " µg/m³ \n"
                           "- 🌪 PM10  : "
         + String(pm10) + " µg/m³\n"
                          "---------------------------------------------------\n"
                          "• ⛅ สภาพอุณหภูมิ/ความชื้น :\n"
                          "-💧Humidity        : "
         + String(humidity) + " %\n"
                              "-🌡Temperature : "
         + String(temperature) + " °C\n"
                                 "* หมายเหตุ : "
         + getTempHumiNote() + "\n"
                               "---------------------------------------------------\n"
                               "•📍 ตำแหน่งปัจจุบัน  (Real-time)\n"
                               "- ↕️ ละติจูด   : "
         + String(latitude, 6) + "\n"
                                 "- ↔️ ลองจิจูด : "
         + String(longitude, 6) + "\n"
                                  "- 🌏 ลิงก์ตำแหน่ง : https://maps.google.com/?q="
         + String(latitude, 6) + "," + String(longitude, 6) + "\n"
                                                              "---------------------------------------------------\n"
                                                              "🥰 ดูแลสุขภาพกันด้วยนะครับ😊\n"
                                                              "---------------------------------------------------\n"
                                                              "ⓘ ข้อมูลชุดนี้มาจากชุดโครงงานระบบวัดค่าฝุ่นละออง PM2.5 ผ่านแอพพลิเคชั่นไลน์ แบบเรียลไทม์";
}
String fullYellowReport() {
  aqi = calculateAQI(pm2_5);
  return "---------------------------------------------------\n"
         "📟 AIR QUALITY REPORT 🌸\n"
         "---------------------------------------------------\n"
         "• 📆 รายงานวันที่ "
         + getDateTH() + "\n"
                         "----------->เวลา : "
         + getTimeTH() + " น.<-----------\n"
                         "🟡 ค่า AQI : "
         + String(aqi) + "\n"
                         "สภาพอากาศ ณ ตอนนี้อยู่ในเกณฑ์ที่ควรลดเวลาอยู่กลางแจ้ง 🤧🥀\n"
                         "---------------------------------------------------\n"
                         "• ✅ คำแนะนำ :\n"
                         "- ควรลดกิจกรรมกลางแจ้ง\n"
                         "- หากจำเป็นควรใส่หน้ากากอนามัย\n"
                         "- สังเกตอาการผิดปกติ เช่น ไอ จาม\n"
                         "---------------------------------------------------\n"
                         "• 🌫 ค่าฝุ่นละออง ปัจจุบัน :\n"
                         "- 👻 PM1.0 : "
         + String(pm1_0) + " µg/m³\n"
                           "- 🤧 PM2.5 : "
         + String(pm2_5) + " µg/m³ \n"
                           "- 🌪 PM10  : "
         + String(pm10) + " µg/m³\n"
                          "---------------------------------------------------\n"
                          "• ⛅ สภาพอุณหภูมิ/ความชื้น :\n"
                          "-💧Humidity        : "
         + String(humidity) + " %\n"
                              "-🌡Temperature : "
         + String(temperature) + " °C\n"
                                 "* หมายเหตุ : "
         + getTempHumiNote() + "\n"
                               "---------------------------------------------------\n"
                               "•📍 ตำแหน่งปัจจุบัน  (Real-time)\n"
                               "- ↕️ ละติจูด   : "
         + String(latitude, 6) + "\n"
                                 "- ↔️ ลองจิจูด : "
         + String(longitude, 6) + "\n"
                                  "- 🌏 ลิงก์ตำแหน่ง : https://maps.google.com/?q="
         + String(latitude, 6) + "," + String(longitude, 6) + "\n"
                                                              "---------------------------------------------------\n"
                                                              "🥰 ดูแลสุขภาพกันด้วยนะครับ😊\n"
                                                              "---------------------------------------------------\n"
                                                              "ⓘ ข้อมูลชุดนี้มาจากชุดโครงงานระบบวัดค่าฝุ่นละออง PM2.5 ผ่านแอพพลิเคชั่นไลน์ แบบเรียลไทม์";
}
String fullOrangeReport() {
  aqi = calculateAQI(pm2_5);
  return "---------------------------------------------------\n"
         "📟 AIR QUALITY REPORT 🌸\n"
         "---------------------------------------------------\n"
         "• 📆 รายงานวันที่ "
         + getDateTH() + "\n"
                         "----------->เวลา : "
         + getTimeTH() + " น.<-----------\n"
                         "🟠 ค่า AQI : "
         + String(aqi) + "\n"
                         "สภาพอากาศ ณ ตอนนี้อยู่ในเกณฑ์ที่เริ่มส่งผลกระทบต่อสุขภาพ😷🕷\n"
                         "---------------------------------------------------\n"
                         "• ✅ คำแนะนำ :\n"
                         "- ลดกิจกรรมกลางแจ้งให้น้อยที่สุด\n"
                         "- ควรใส่หน้ากากป้องกันฝุ่น PM2.5\n"
                         "- ปิดประตูหน้าต่างเพื่อลดฝุ่นจากด้านนอก\n"
                         "---------------------------------------------------\n"
                         "• 🌫 ค่าฝุ่นละออง ปัจจุบัน :\n"
                         "- 👻 PM1.0 : "
         + String(pm1_0) + " µg/m³\n"
                           "- 😷 PM2.5 : "
         + String(pm2_5) + " µg/m³ \n"
                           "- 🌪 PM10  : "
         + String(pm10) + " µg/m³\n"
                          "---------------------------------------------------\n"
                          "• ⛅ สภาพอุณหภูมิ/ความชื้น :\n"
                          "-💧Humidity        : "
         + String(humidity) + " %\n"
                              "-🌡Temperature : "
         + String(temperature) + " °C\n"
                                 "* หมายเหตุ : "
         + getTempHumiNote() + "\n"
                               "---------------------------------------------------\n"
                               "•📍 ตำแหน่งปัจจุบัน  (Real-time)\n"
                               "- ↕️ ละติจูด   : "
         + String(latitude, 6) + "\n"
                                 "- ↔️ ลองจิจูด : "
         + String(longitude, 6) + "\n"
                                  "- 🌏 ลิงก์ตำแหน่ง : https://maps.google.com/?q="
         + String(latitude, 6) + "," + String(longitude, 6) + "\n"
                                                              "---------------------------------------------------\n"
                                                              "🥰 ดูแลสุขภาพกันด้วยนะครับ😊\n"
                                                              "---------------------------------------------------\n"
                                                              "ⓘ ข้อมูลชุดนี้มาจากชุดโครงงานระบบวัดค่าฝุ่นละออง PM2.5 ผ่านแอพพลิเคชั่นไลน์ แบบเรียลไทม์";
}
String fullRedReport() {
  aqi = calculateAQI(pm2_5);
  return "---------------------------------------------------\n"
         "📟 AIR QUALITY REPORT 🌸\n"
         "---------------------------------------------------\n"
         "• 📆 รายงานวันที่ "
         + getDateTH() + "\n"
                         "------------>เวลา : "
         + getTimeTH() + " น.<------------\n"
                         "🔴 ค่า AQI : "
         + String(aqi) + "\n"
                         "สภาพอากาศอยู่ในระดับมีผลกระทบต่อสุขภาพอย่างชัดเจน 😷🚨\n"
                         "---------------------------------------------------\n"
                         "• 🚨 คำแนะนำแบบเร่งด่วน :\n"
                         "- หลีกเลี่ยงการออกนอกอาคาร\n"
                         "- กลุ่มเสี่ยงควรอยู่ในพื้นที่ปลอดภัย\n"
                         "- หากมีอาการผิดปกติควรพบแพทย์\n"
                         "---------------------------------------------------\n"
                         "• 🌫 ค่าฝุ่นละออง ปัจจุบัน :\n"
                         "- 👻 PM1.0 : "
         + String(pm1_0) + " µg/m³\n"
                           "- 😷 PM2.5 : "
         + String(pm2_5) + " µg/m³ \n"
                           "- 🌪 PM10  : "
         + String(pm10) + " µg/m³\n"
                          "---------------------------------------------------\n"
                          "• ⛅ สภาพอุณหภูมิ/ความชื้น :\n"
                          "-💧Humidity        : "
         + String(humidity) + " %\n"
                              "-🌡Temperature : "
         + String(temperature) + " °C\n"
                                 "* หมายเหตุ : "
         + getTempHumiNote() + "\n"
                               "---------------------------------------------------\n"
                               "•📍 ตำแหน่งปัจจุบัน  (Real-time)\n"
                               "- ↕️ ละติจูด   : "
         + String(latitude, 6) + "\n"
                                 "- ↔️ ลองจิจูด : "
         + String(longitude, 6) + "\n"
                                  "- 🌏 ลิงก์ตำแหน่ง : https://maps.google.com/?q="
         + String(latitude, 6) + "," + String(longitude, 6) + "\n"
                                                              "---------------------------------------------------\n"
                                                              "🫂 ดูแลสุขภาพกันด้วยนะครับ🦋\n"
                                                              "---------------------------------------------------\n"
                                                              "ⓘ ข้อมูลชุดนี้มาจากชุดโครงงานระบบวัดค่าฝุ่นละออง PM2.5 ผ่านแอพพลิเคชั่นไลน์ แบบเรียลไทม์";
}
#define BUTTON_PIN 27

unsigned long buttonPressTime = 0;
bool buttonHandled = false;

bool requestFullReport = false;
unsigned long lastButtonSend = 0;
const unsigned long BUTTON_COOLDOWN = 60000;

#define GPS_RX 16
#define GPS_TX 17
#define PM25_DANGER 50

HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

bool hasGPS = false;
//===========================================

// ================= สี =================
#define COLOR_NORMAL ST77XX_BLACK
#define COLOR_WARNING 0xFFE5
#define COLOR_DANGER 0xF800

// ================= TFT =================
#define TFT_CS 5
#define TFT_RST 25
#define TFT_DC 19
#define TFT_SCLK 18
#define TFT_MOSI 23
Adafruit_ST7735 tft = Adafruit_ST7735(
  TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ================= FIXED NUMBER =================
void printFixedNumber(int x, int y, int v) {
  tft.setCursor(x, y);

  if (v < 1000) tft.print(" ");
  if (v < 100) tft.print(" ");
  if (v < 10) tft.print(" ");

  tft.print(v);
}
// ================= PMS3003 =================
#define PMS_RX 21
HardwareSerial pmsSerial(2);

// ================= DHT22 =================
#define DHTPIN 22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);


// ================= WiFi + LINE =================
// ===== WIFI 1 : บ้าน =====
const char* ssid1 = "....";
const char* password1 = "....";

// ===== WIFI 2 : Hotspot มือถือ =====
const char* ssid2 = "...";
const char* password2 = "....";


String LINE_CHANNEL_ACCESS_TOKEN = ".....";
String LINE_USER_ID = "...";


// ================= Timer =================
unsigned long bootTime;
unsigned long lastLineSend = 0;
const unsigned long MIN_LINE_INTERVAL = 60000; 
unsigned long retryAfter429 = 0;

const unsigned long BOOT_DELAY = 30000;  


bool firstLineSent = false;


// ================= LINE =================
bool sendLineMessage(String text) {
  
  if (millis() - lastLineSend < MIN_LINE_INTERVAL) {
    Serial.println("⏳ Skip LINE (too frequent)");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, "https://api.line.me/v2/bot/message/push");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + LINE_CHANNEL_ACCESS_TOKEN);

  StaticJsonDocument<1024> doc;
  doc["to"] = LINE_USER_ID;
  JsonArray msg = doc.createNestedArray("messages");
  msg.createNestedObject()["type"] = "text";
  msg[0]["text"] = text;

  String payload;
  serializeJson(doc, payload);

  int code = http.POST(payload);
  if (code == 200) {
    lastLineSend = millis();  
  }
  Serial.println("📨 LINE status = " + String(code));

  http.end();

  // ถ้า 200 = สำเร็จ
  return (code == 200);
}
// ================= URL ENCODE (FIXED) =================
String urlEncode(String str) {
  String encoded = "";
  char bufHex[4];
  for (int i = 0; i < str.length(); i++) {
    unsigned char c = str.charAt(i);  
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += (char)c;
    } else {
      sprintf(bufHex, "%%%02X", c);
      encoded += bufHex;
    }
  }
  return encoded;
}
// ================= GOOGLE SHEET =================
#define GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/AKfycbw5LSNClCzOPQK4XeSiYUM-PRM3WgsaEmx1XT8MgATZQLbVQgUdyuRNJRZaBJcM_2QkUw/exec"
void sendToGoogleSheet() {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String mapLink;
  if (hasGPS) {
    mapLink = "https://maps.google.com/?q=" + String(latitude, 6) + "," + String(longitude, 6);
  } else {
    mapLink = "No_GPS";
  }
  String url = String(GOOGLE_SCRIPT_URL) + "?date=" + urlEncode(getDateTH()) + "&time=" + urlEncode(getTimeTH()) + "&pm1=" + String(pm1_0) + "&pm25=" + String(pm2_5) + "&aqi=" + String(aqi) + "&pm10=" + String(pm10) + "&temp=" + String(temperature, 1) + "&humi=" + String(humidity, 1) + "&map=" + urlEncode(mapLink);
  Serial.println("SEND URL:");
  Serial.println(url);
  http.begin(client, url);
  int httpCode = http.GET();
  Serial.println("📊 Google Sheet status: " + String(httpCode));
  http.end();
}
// ================= PMS =================
bool readPMSFrame(uint8_t* buffer) {
  while (pmsSerial.available() >= 32) {
    if (pmsSerial.peek() == 0x42) {
      pmsSerial.read();  // 0x42
      if (pmsSerial.peek() == 0x4D) {
        pmsSerial.read();  // 0x4D
        buffer[0] = 0x42;
        buffer[1] = 0x4D;
        for (int i = 2; i < 32; i++) {
          buffer[i] = pmsSerial.read();
        }
        return true;
      }
    }
    pmsSerial.read();  // ทิ้ง byte ที่ไม่ใช่ header
  }
  return false;
}
void handleButton() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  if (pressed) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis();
      buttonHandled = false;
    }
    // กดค้าง 1 วินาที
    if (!buttonHandled && millis() - buttonPressTime >= 3000) {
      // กันสแปม
      if (millis() - lastButtonSend >= BUTTON_COOLDOWN) {
        requestFullReport = true;
        Serial.println("Full report requested");
      } else {
        Serial.println("Cooldown active");
      }
      buttonHandled = true;
    }
  } else {
    buttonPressTime = 0;
    buttonHandled = false;
  }
}
void drawBackground() {
  if (dangerMode) {
    tft.drawRGBBitmap(0, 0, epd_bitmap_allArray[IMG_DANGER], 128, 160);
  } else {
    tft.drawRGBBitmap(0, 0, epd_bitmap_allArray[IMG_NORMAL], 128, 160);
  }
}
void drawScreen() {
  if (dangerMode) {
    tft.drawRGBBitmap(0, 0, epd_bitmap_allArray[IMG_DANGER], 128, 160);
  } else {
    tft.drawRGBBitmap(0, 0, epd_bitmap_allArray[IMG_NORMAL], 128, 160);
  }
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(30, 10);
  tft.print("Air Quality");

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);

  // ---------- PM1.0 ----------
  tft.setCursor(10, 40);
  tft.print("PM1.0:");
  printFixedNumber(80, 40, pm1_0);

  // ---------- PM2.5 ----------
  tft.setCursor(10, 65);
  tft.print("PM2.5:");
  printFixedNumber(80, 65, pm2_5);

  // ---------- PM10 ----------
  tft.setCursor(10, 90);
  tft.print("PM10 :");
  printFixedNumber(80, 90, pm10);


  tft.setTextSize(1);
  tft.setCursor(10, 120);
  tft.printf("Humi: %.0f%%", humidity);

  tft.setCursor(10, 135);
  tft.printf("Temp: %.1fC", temperature);
}
void drawSendingScreen() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(10, 50);
  tft.print("Sending...");

  tft.setTextSize(1);
  tft.setCursor(10, 80);
  tft.print("Please wait");
}
// ================= LINE : DANGER =================
void sendLineDanger() {
  sendLineMessage(buildShortMessage());
}
// ================= LINE : BACK TO NORMAL =================
void sendLineBackToNormal() {
  sendLineMessage(buildShortMessage());
}
void updateBuzzer() {
  if (!buzzerActive) return;
  unsigned long now = millis();
  if (buzzerState == false) {
    if (now - lastBuzzerToggle >= beepOffTime) {
      digitalWrite(BUZZER_RELAY_PIN, LOW);  // เปิดเสียง
      buzzerState = true;
      lastBuzzerToggle = now;
    }
  } else {
    if (now - lastBuzzerToggle >= beepOnTime) {
      digitalWrite(BUZZER_RELAY_PIN, HIGH);  // ปิดเสียง
      buzzerState = false;
      lastBuzzerToggle = now;
      beepCount++; 
      if (beepCount >= maxBeeps) {
        buzzerActive = false;  
        digitalWrite(BUZZER_RELAY_PIN, HIGH);
      }
    }
  }
}
String getTempHumiNote() {
  if (temperature >= 35 && humidity >= 70) {
    return "อากาศร้อนจัด และมีความชื้นสูง อาจเกิดอาการอ่อนเพลีย";
  } else if (temperature >= 32 && humidity >= 70) {
    return "อากาศร้อน และความชื้นสูง อาจทำให้รู้สึกอึดอัด";
  } else if (temperature >= 32) {
    return "อุณหภูมิค่อนข้างร้อน ควรดื่มน้ำให้เพียงพอ";
  } else if (humidity >= 80) {
    return "ความชื้นสูง อาจทำให้หายใจไม่สบาย";
  } else {
    return "อุณหภมิ/ความชื้นอยู่ในระดับปกติ";
  }
}
void connectWiFi() {
  // ===== รีเซ็ต WiFi เต็มระบบ =====
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.println("Connecting to WiFi 1...");
  WiFi.begin(ssid1, password1);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected WiFi 1");
    return;
  }
  // ===== ถ้า WiFi1 ไม่ติด =====
  Serial.println("\nWiFi1 failed, reset before Hotspot...");
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.println("Trying Hotspot...");
  WiFi.begin(ssid2, password2);
  startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected Hotspot");
  } else {
    Serial.println("\n❌ WiFi connect failed");
  }
}
int calculateAQI(int pm) {
  if (pm <= 12) return map(pm, 0, 12, 0, 50);
  else if (pm <= 35) return map(pm, 13, 35, 51, 100);
  else if (pm <= 55) return map(pm, 36, 55, 101, 150);
  else if (pm <= 150) return map(pm, 56, 150, 151, 200);
  else if (pm <= 250) return map(pm, 151, 250, 201, 300);
  else return 301;  // อันตรายมาก
}
String buildFullReportMessage() {
  if (aqi <= 50) {
    return fullGreenReport();
  } else if (aqi <= 100) {
    return fullYellowReport();
  } else if (aqi <= 150) {
    return fullOrangeReport();
  } else {
    return fullRedReport();
  }
}
// ================= SETUP =================
void setup() {
  pinMode(BUZZER_RELAY_PIN, OUTPUT);
  digitalWrite(BUZZER_RELAY_PIN, HIGH);
  delay(50);
  Serial.begin(115200);
  startTime = millis();
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  // ===== ภาพ BOOT =====
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRGBBitmap(0, 0, epd_bitmap_allArray[IMG_BOOT], 128, 160);
  pmReady = false;
  connectWiFi();
  lastLineSend = millis() - MIN_LINE_INTERVAL;
  // ===== SERVO INIT =====
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo1.write(30);
  servo2.write(150);
  servoIsUp = false;
  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, -1);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  dht.begin();
  bootTime = millis();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for time");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" OK");
  sendLineMessage(buildWelcomeMessage());
  delay(65000);  // รอ 65 วินาที (กัน LINE 429)
  sendLineMessage(buildProjectInfoMessage());
}
void drawPM(float pm1, float pm25, float pm10) {
  tft.fillRect(0, 50, 160, 60, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 50);
  tft.print("PM1.0:");
  tft.setCursor(90, 50);
  tft.print((int)pm1);
  tft.setCursor(0, 70);
  tft.print("PM2.5:");
  tft.setCursor(90, 70);
  tft.print((int)pm25);
  tft.setCursor(0, 90);
  tft.print("PM10 :");
  tft.setCursor(90, 90);
  tft.print((int)pm10);
}
// ================= LOOP =================
void loop() {
  // ================= BUTTON =================
  handleButton();
  // ===== ส่งรายงานแบบยาวจากปุ่ม =====
  if (requestFullReport) {
    requestFullReport = false;
    sendingScreen = true;
    drawSendingScreen();
    bool sent = sendLineMessage(buildFullReportMessage());
    if (sent) {
      lastButtonSend = millis();
      Serial.println("Full report sent");
    }
    sendingScreen = false;
    drawBackground();
  }
  // ================= READ GPS =================
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
  if (gps.location.isUpdated()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    hasGPS = true;
    Serial.print("📡 GPS FIXED: ");
    Serial.print(latitude, 6);
    Serial.print(", ");
    Serial.println(longitude, 6);
  }
  // ================= WIFI RECONNECT =================
  if (millis() - lastWifiCheck >= 10000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi reconnect...");
      connectWiFi();
    }
  }
  // ================= READ PMS (ทุก 1 วิ) =================
  static unsigned long lastPMSRead = 0;
  if (millis() - lastPMSRead >= 1000) {
    lastPMSRead = millis();
    uint8_t buffer[32];
    if (readPMSFrame(buffer)) {
      pm1_0 = (buffer[10] << 8) | buffer[11];
      pm2_5 = (buffer[12] << 8) | buffer[13];
      pm10 = (buffer[14] << 8) | buffer[15];
      aqi = calculateAQI(pm2_5);  
      hasPMData = true;
    }
    // ===== ครั้งแรกที่ได้ค่า PM =====
    if (!pmReady) {
      pmReady = true;
      if (pm2_5 >= PM25_DANGER_ON) {
        dangerMode = true;
      } else {
        dangerMode = false;
      }
      aqi = calculateAQI(pm2_5);
      drawBackground();  // เปลี่ยนจาก BOOT → NORMAL หรือ DANGER
    }
    Serial.print("🌫 PM2.5 = ");
    Serial.println(pm2_5);
  }
  // ================= READ DHT (ทุก 2 วิ) =================
  static unsigned long lastDHTRead = 0;
  if (millis() - lastDHTRead >= 2000) {
    lastDHTRead = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h)) humidity = h;
    if (!isnan(t)) temperature = t;
    Serial.print("💧 H = ");
    Serial.print(humidity);
    Serial.print(" | 🌡 T = ");
    Serial.println(temperature);
  }
  // ================= PM2.5 + LINE (ไม่สแปม) =================
  static bool lastDangerMode = false;
  if (!dangerMode && aqi >= 151) {
    dangerMode = true;
  } else if (dangerMode && aqi <= 100) {
    dangerMode = false;
  }
  if (dangerMode && !lastDangerMode) {
    // ===== SERVO UP =====
    servo1.write(120);
    servo2.write(60);
    servoIsUp = true;
    buzzerActive = true;
    buzzerState = false;
    lastBuzzerToggle = millis();
    beepCount = 0;  // สำคัญมาก!
    sendLineDanger();
  }
  if (!dangerMode && lastDangerMode) {
    // ===== SERVO DOWN =====
    servo1.write(30);
    servo2.write(150);
    servoIsUp = false;
    sendLineBackToNormal();
    dangerLineSent = false;
  }
  lastDangerMode = dangerMode;
  // ===== หยุดบัสเซอร์ทันทีเมื่อค่าปลอดภัย =====
  if (!dangerMode && buzzerActive) {
    buzzerActive = false;
    digitalWrite(BUZZER_RELAY_PIN, HIGH);  // ปิดรีเลย์ทันที
    buzzerState = false;
    beepCount = 0;
    Serial.println("BUZZER STOP (safe)");
  }
  updateBuzzer();
  // ================= รายงานทุก 5 นาที =================
  if (!dangerMode && hasPMData && millis() - lastLineNotify >= LINE_INTERVAL) {
    lastLineNotify = millis();
    sendLineMessage(buildShortMessage());
  }
  // ================= TFT UPDATE =================
  if (!sendingScreen && pmReady && millis() - lastTFTUpdate >= TFT_INTERVAL) {
    lastTFTUpdate = millis();
    drawScreen();
  }
  // ================= GOOGLE SHEET =================
  if (millis() - lastSheetSend >= SHEET_INTERVAL) {
    lastSheetSend = millis();
    sendToGoogleSheet();
  }
}
