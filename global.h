/*  Controlling Jarolift TDEF 433MHZ radio shutters via ESP8266 and CC1101 Transceiver Module in asynchronous mode.
    Copyright (C) 2017-2018 Steffen Hille et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <simpleDSTadjust.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <EEPROM.h>


#define PROGRAM_VERSION "v0.8"

#define EEPROM_SIZE            1320               // must not exceed 4096 bytes
#define ACCESS_POINT_NAME      "Jarolift-Dongle"  // default SSID for Admin-Mode
#define ACCESS_POINT_PASSWORD  "12345678"         // default WLAN password for Admin-Mode
#define AdminTimeOut           180                // Defines the time in seconds, when the Admin-Mode will be disabled
#define MQTT_Reconnect_Interval 30000             // try connect to MQTT server very X milliseconds
#define NTP_SERVERS            "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org" // List of up to 3 NTP servers
#define TIMEZONE               +1                 // difference localtime to UTC/GMT in hours
#define TIMEZONE_DAYLIGHT       "CEST"            // abbreviation for daylight saving time zone
#define TIMEZONE_STANDARD       "CET"             // abbreviation for standard time zone

struct dstRule startRule = {TIMEZONE_DAYLIGHT, Last, Sun, Mar, 2, 3600};  // beginning of daylight time
struct dstRule endRule   = {TIMEZONE_STANDARD, Last, Sun, Oct, 2, 0};     // beginning of standard time
simpleDSTadjust dstAdjusted(startRule, endRule);  // Setup simpleDSTadjust Library rules

ESP8266WebServer server(80);                      // The Webserver
WiFiClient espClient;
PubSubClient mqtt_client(espClient);              // mqtt client instance
long mqttLastConnectAttempt = 0;                  // timepoint of last connect attempt in milliseconds
WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

boolean AdminEnabled = false;                     // Admin-Mode opens AccessPoint for configuration
int AdminTimeOutCounter = 0;                      // Counter for Disabling the Admin-Mode
boolean wifi_disconnect_log = true;               // Flag to avoid repeated logging of disconnect events
int led_pin = LED_BUILTIN;                        // GPIO Pin number for LED
Ticker tkHeartBeat;                               // Timer for HeartBeat
unsigned short devcnt = 0x0;                      // Initial 16Bit countervalue, stored in EEPROM and incremented once every time a command is send
int cntadr = 110;                                 // EEPROM address where the 16Bit counter is stored.

String web_cmd = "";                              // trigger to run a command whenever a action button has been pressed on the web interface
int web_cmd_channel;                              // keeps the respective channel ID for the web_cmd

#define NUM_WEB_LOG_MESSAGES 42                   // number of messages in the web-UI log page
String web_log_message[NUM_WEB_LOG_MESSAGES];
uint16_t web_log_message_nextfree = 0;
boolean web_log_message_rotated = false;
boolean web_log_message_newline = true;

// boolean settings which will later be configurable through WebUI
boolean debug_mqtt = false;
boolean debug_webui = true;
boolean debug_log_radio_receive_all = false;
boolean mqtt_send_radio_receive_all = true;

#define MAX_CHANNELS  16
struct strConfig {
  uint16_t  cfgVersion;
  String    ssid;
  String    password;
  byte      ip[4];
  byte      netmask[4];
  byte      gateway[4];
  boolean   dhcp;
  byte      mqtt_broker_addr[4];
  String    mqtt_broker_port;
  String    mqtt_broker_client_id;
  String    mqtt_broker_username;
  String    mqtt_broker_password;
  String    mqtt_devicetopic;
  String    master_msb_str;
  String    master_lsb_str;
  uint32_t  master_msb_num;
  uint32_t  master_lsb_num;
  boolean   learn_mode;     // If set to true, regular learn method is used (up+down, followed by stop), otherwise another method for older versions of Jarolift motors is used.
  boolean   shade_support;  // If set to true, built-in shade mode is used. Otherwise, shading is emulated by DOWN & STOP commands with runtime.
  boolean   shade_enable;   // If set to true, shading is enabled & shutters move to the shade position at the configured time.
  uint16_t  shade_runtime[MAX_CHANNELS];  // runtime until shutter reaches shade position - used for motors without built-in shade mode
  byte      flags[MAX_CHANNELS];          // flags for shutter channel settings / features
  String    serial;         // starting serial number as string
  uint32_t  serial_number;  // starting serial number as integer
  String    channel_name[MAX_CHANNELS];
  // temporary values, for web ui, not to store in EEPROM
  String    mqtt_devicetopic_new = ""; // needed to figure out if the devicetopic has been changed
  String    new_serial = "";
  String    new_devicecounter = "";
  boolean   set_and_generate_serial = false;
  boolean   set_devicecounter = false;
} config;

// bitmasks for channel flags
const byte CHANNEL_FLAG_AUTO_UPDOWN = 0x01;
const byte CHANNEL_FLAG_AUTO_SHADE  = 0x02;

// API for sending shutter commands
enum ShutterCmd {
  CMD_IDLE,
  CMD_UP,
  CMD_DOWN,
  CMD_STOP,
  CMD_SETSHADE,
  CMD_SHADE,
  CMD_LEARN,
  CMD_UPDOWN,
};
void sendCmd(ShutterCmd cmd, unsigned channel);
ShutterCmd string2ShutterCmd(const char* str);

//####################################################################
// Initalize log message array with empty strings
//####################################################################
void InitLog() {
  for (unsigned i = 0; i < NUM_WEB_LOG_MESSAGES; ++i) {
    web_log_message[i] = "";
  }
} // void InitLog

//####################################################################
// Function to write Log to both, serial and Weblog
//####################################################################
void WriteLog(String msg, boolean new_line = false) {
  // web_log_message[] is an array of strings, used as a circular buffer
  // web_log_message_nextfree points to the next free-to-use buffer line
  // when web_log_message_nextfree becomes larger than NUM_WEB_LOG_MESSAGES,
  // it is reset to 0 and web_log_message_rotated=true
  // web_log_message_newline indicates whether we are at the beginning of a line or not

  if (web_log_message_newline) {
    char *dstAbbrev;
    time_t now = dstAdjusted.time(&dstAbbrev);
    struct tm * timeinfo = localtime(&now);
    char buffer[30];
    strftime (buffer, 30, "%Y-%m-%d %T", timeinfo);
    web_log_message[web_log_message_nextfree] = (String)buffer + " " + dstAbbrev + " " + msg;
    Serial.print((String)buffer + " " + dstAbbrev + " " + msg);
    web_log_message_newline = false;
  } else {
    web_log_message[web_log_message_nextfree] += " " + msg;
    Serial.print(" " + msg);
  }
  if (new_line == true) {
    web_log_message_nextfree++;
    if (web_log_message_nextfree >= NUM_WEB_LOG_MESSAGES) {
      web_log_message_nextfree = 0;
      web_log_message_rotated = true;
    }
    Serial.println();
    web_log_message_newline = true;
  }
} // void WriteLog

//####################################################################
// Function to connect to Wifi
//####################################################################
void ConfigureWifi() {
  WriteLog("[INFO] - WiFi connecting to", false);
  WriteLog(config.ssid, true);
  WiFi.mode(WIFI_STA);
  WiFi.begin (config.ssid.c_str(), config.password.c_str());
  if (!config.dhcp) {
    WiFi.config(IPAddress(config.ip[0], config.ip[1], config.ip[2], config.ip[3] ),  IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3] ) , IPAddress(config.netmask[0], config.netmask[1], config.netmask[2], config.netmask[3] ));
  }
  wifi_disconnect_log = true;
} // void ConfigureWifi

//################################################################
// Function to write newest version of configuration into EEPROM
//################################################################
void WriteConfig() {
  WriteLog("[INFO] - write config to EEPROM", true);
  EEPROM.write(120, 'C');
  EEPROM.write(121, 'f');
  EEPROM.write(122, 'g');
  EEPROM.put(123, config.cfgVersion);

  EEPROM.write(128, config.dhcp);

  EEPROM.write(144, config.ip[0]);
  EEPROM.write(145, config.ip[1]);
  EEPROM.write(146, config.ip[2]);
  EEPROM.write(147, config.ip[3]);

  EEPROM.write(148, config.netmask[0]);
  EEPROM.write(149, config.netmask[1]);
  EEPROM.write(150, config.netmask[2]);
  EEPROM.write(151, config.netmask[3]);

  EEPROM.write(152, config.gateway[0]);
  EEPROM.write(153, config.gateway[1]);
  EEPROM.write(154, config.gateway[2]);
  EEPROM.write(155, config.gateway[3]);

  EEPROM.write(156, config.mqtt_broker_addr[0]);
  EEPROM.write(157, config.mqtt_broker_addr[1]);
  EEPROM.write(158, config.mqtt_broker_addr[2]);
  EEPROM.write(159, config.mqtt_broker_addr[3]);

  WriteStringToEEPROM(164, config.ssid);
  WriteStringToEEPROM(196, config.password);

  WriteStringToEEPROM(262, config.mqtt_broker_port);
  WriteStringToEEPROM(270, config.master_msb_str);
  WriteStringToEEPROM(330, config.master_lsb_str);
  EEPROM.write(350, config.learn_mode);
  EEPROM.write(351, config.shade_support);
  EEPROM.write(352, config.shade_enable);
  WriteStringToEEPROM(366, config.serial);

  WriteStringToEEPROM(375, config.mqtt_broker_client_id);
  WriteStringToEEPROM(400, config.mqtt_broker_username);
  WriteStringToEEPROM(450, config.mqtt_broker_password);
  WriteStringToEEPROM(1300, config.mqtt_devicetopic);

  for (unsigned i = 0; i < MAX_CHANNELS; ++i) {
    WriteStringToEEPROM(500 + i * 50, config.channel_name[i]);
    WriteStringToEEPROM(525 + i * 50, String(config.shade_runtime[i]));
    EEPROM.write(531 + i * 50, config.flags[i]);
  }

  EEPROM.commit();
  delay(1000);
} // void WriteConfig

void WriteConfigShadeRuntime(unsigned channel) {
  // helper function for writing shade runtime without committing to EEPROM on each change
  if (channel < MAX_CHANNELS) {
    WriteStringToEEPROM(525 + channel * 50, String(config.shade_runtime[channel]));
  }
}

void WriteChannelFlags(unsigned channel) {
  // helper function for writing channel flags without committing to EEPROM on each click
  if (channel < MAX_CHANNELS) {
    EEPROM.write(531 + channel * 50, config.flags[channel]);
  }
}

//####################################################################
// Function to read configuration from EEPROM
//####################################################################
boolean ReadConfig() {
  config.cfgVersion = 0;
  char cfgVersBuf[3];

  WriteLog("[INFO] - read config from EEPROM...", false);

  for (unsigned i = 0; i < sizeof(cfgVersBuf)/sizeof(cfgVersBuf[0]); ++i) {
    cfgVersBuf[i] = EEPROM.read(120 + i);
  }
  if (memcmp(cfgVersBuf, "CFG", 3) == 0) {
    config.cfgVersion = 1;
  } else if (memcmp(cfgVersBuf, "Cfg", 3) == 0) {
    EEPROM.get(123, config.cfgVersion);
  }
  if (config.cfgVersion == 0) {
    WriteLog("no configuration found!", true);
    return false;
  }
  
  WriteLog("version " + (String)config.cfgVersion + " found", true);
  if (config.cfgVersion <= 2) {
    // read config parts up to version 2
    config.dhcp  = EEPROM.read(128);
    config.ip[0] = EEPROM.read(144);
    config.ip[1] = EEPROM.read(145);
    config.ip[2] = EEPROM.read(146);
    config.ip[3] = EEPROM.read(147);
    config.netmask[0] = EEPROM.read(148);
    config.netmask[1] = EEPROM.read(149);
    config.netmask[2] = EEPROM.read(150);
    config.netmask[3] = EEPROM.read(151);
    config.gateway[0] = EEPROM.read(152);
    config.gateway[1] = EEPROM.read(153);
    config.gateway[2] = EEPROM.read(154);
    config.gateway[3] = EEPROM.read(155);
    config.mqtt_broker_addr[0] = EEPROM.read(156);
    config.mqtt_broker_addr[1] = EEPROM.read(157);
    config.mqtt_broker_addr[2] = EEPROM.read(158);
    config.mqtt_broker_addr[3] = EEPROM.read(159);

    config.ssid             = ReadStringFromEEPROM(164, 32);
    config.password         = ReadStringFromEEPROM(196, 64);
    config.mqtt_broker_port = ReadStringFromEEPROM(262, 5);
    config.master_msb_str   = ReadStringFromEEPROM(270, 10);
    config.master_lsb_str   = ReadStringFromEEPROM(330, 10);
    config.learn_mode       = EEPROM.read(350);
    config.shade_support    = EEPROM.read(351);
    config.shade_enable     = EEPROM.read(352);
    config.serial           = ReadStringFromEEPROM(366, 8);

    config.mqtt_broker_client_id  = ReadStringFromEEPROM(375, 25);
    config.mqtt_broker_username   = ReadStringFromEEPROM(400, 25);
    config.mqtt_broker_password   = ReadStringFromEEPROM(450, 25);

    for (unsigned i = 0; i < MAX_CHANNELS; ++i) {
      config.channel_name[i]  = ReadStringFromEEPROM(500 + i * 50, 25);
      config.shade_runtime[i] = ReadStringFromEEPROM(525 + i * 50, 6).toInt();
      config.flags[i]         = EEPROM.read(531 + i * 50);
    }
  }
  if (config.cfgVersion == 2) {
    // read config parts of version 2
    config.mqtt_devicetopic = ReadStringFromEEPROM(1300, 20);
  } else {
    // upgrade config to version 2
    config.mqtt_devicetopic = "jarolift"; // default devicetopic
    config.cfgVersion = 2;
    // clear EEPROM space
    for (unsigned addr = 120; addr < EEPROM_SIZE; ++addr) {
      EEPROM.write(addr, 0);
    }
    WriteLog("[INFO] - config update to version 2 - mqtt_devicetopic set to " + config.mqtt_devicetopic, true);
    WriteConfig();
  }

  // check if config.serial is hexadecimal
  // if necessary, convert decimal to hexadecimal
  if ((config.serial[0] == '0') && (config.serial[1] == 'x')) {
    // config.serial is hex
    // Serial is 28 bits
    // string serial stores only highest 24 bits,
    // add lowest byte with a shift operation for config.serial_number
    config.serial_number = strtol(config.serial.c_str(), NULL, 16) << 4;
  } else {
    // config.serial is NOT hex
    config.serial_number = strtol(config.serial.c_str(), NULL, 10);
    // Serial is 28 bits.
    // string serial stores only highest 24 bits,
    // remove lowest 4 bits with a shift operation
    char buf[11];
    snprintf(buf, sizeof(buf)/sizeof(buf[0]), "0x%06x", (config.serial_number >> 4));
    config.serial = buf;
    Serial.printf("convert config.serial to hex: %08u = 0x%06x\n", config.serial_number, config.serial_number);
    Serial.println("config.serial: " + config.serial);
    WriteLog("[INFO] - config version 2 - convert serial decimal->hexadecimal", true);
    WriteConfig();
  }

  char buf[11];
  config.master_msb_str.toCharArray(buf, sizeof(buf) / sizeof(buf[0]));
  config.master_msb_num = (int)strtol(buf, NULL, 16);
  config.master_lsb_str.toCharArray(buf, sizeof(buf) / sizeof(buf[0]));
  config.master_lsb_num = (int)strtol(buf, NULL, 16);

  return true;
} // boolean ReadConfig

//############################################################################
// Function to initialize configuration, either defaults or read from EEPROM
//############################################################################
void InitializeConfigData() {
  char chipIdString[10];
  sprintf(chipIdString, "-%08x", ESP.getChipId());

  // apply default config if saved configuration not yet exist
  if (!ReadConfig()) {
    // DEFAULT CONFIG
    config.cfgVersion = 2;
    config.ssid = "MYSSID";
    config.password = "MYPASSWORD";
    config.dhcp = true;
    config.ip[0] = 192; config.ip[1] = 168; config.ip[2] = 1; config.ip[3] = 100;
    config.netmask[0] = 255; config.netmask[1] = 255; config.netmask[2] = 255; config.netmask[3] = 0;
    config.gateway[0] = 192; config.gateway[1] = 168; config.gateway[2] = 1; config.gateway[3] = 1;
    config.mqtt_broker_addr[0] = 0; config.mqtt_broker_addr[1] = 0; config.mqtt_broker_addr[2] = 0; config.mqtt_broker_addr[3] = 0;
    config.mqtt_broker_port = "1883";
    config.mqtt_broker_client_id = "JaroliftDongle";
    config.mqtt_broker_client_id += chipIdString;
    config.mqtt_devicetopic = "jarolift";
    config.master_msb_str = "0x12345678";
    config.master_lsb_str = "0x12345678";
    config.learn_mode = true;
    config.shade_support = false;
    config.shade_enable  = false;
    config.serial = "12345600";
    for (unsigned i = 0; i < MAX_CHANNELS; ++i) {
      config.channel_name[i]  = "";
      config.shade_runtime[i] = 0;
      config.flags[i]         = 0;
    }
    WriteConfig();
    WriteLog("[INFO] - default config applied", true);
  }

  // check if mqtt client-ID needs migration to new unique ID
  if (config.mqtt_broker_client_id == "JaroliftDongle") {
    config.mqtt_broker_client_id += chipIdString;
    WriteLog("[INFO] - mqtt_broker_client_id changed to " + config.mqtt_broker_client_id, true);
    WriteConfig();
  }
} // void InitializeConfigData

//####################################################################
// Callback function for Ticker
//####################################################################
void Admin_Mode_Timeout() {
  AdminTimeOutCounter++;
} // void Admin_Mode_Timeout

//####################################################################
// Callback function for LED HeartBeat & EEPROM commit
//####################################################################
boolean highPulse = true;
#define HEART_BEAT_CYCLE 60                      // HeartBeat cycle in seconds
#define EEPROM_COMMIT_CYCLE 3600                  // cyclic EEPROM commit (if needed)

void HeartBeat() {
  static unsigned cnt;
  if (cnt < EEPROM_COMMIT_CYCLE / HEART_BEAT_CYCLE) {
    cnt++;
  } else {
    // cyclic EEPROM commit (does nothing if not dirty) to avoid excessive flash wear
    if (EEPROM.commit()) {
      cnt = 0;
    } else {
      // retry EEPROM write on every heartbeat cycle
      WriteLog("[ERR ] - cyclic EEPROM commit failed", true);
    }
  }
  float pulse_on  = 0.05;                        // LED on for 50 milliseconds in normal mode
  float pulse_off = HEART_BEAT_CYCLE - pulse_on;

  if (WiFi.status() != WL_CONNECTED) {
    pulse_on  = HEART_BEAT_CYCLE / 2;            // LED on for 2 seconds, 2 seconds off while waiting for WiFi connect
    pulse_off = pulse_on;
  }

  if (AdminEnabled) {
    AdminTimeOutCounter++;
    pulse_off = 0.05;
    pulse_on  = HEART_BEAT_CYCLE - pulse_off;    // LED off for 50 milliseconds in admin mode
  }

  if (highPulse) {
    tkHeartBeat.attach(pulse_on, HeartBeat);
  } else {
    tkHeartBeat.attach(pulse_off, HeartBeat);
  }

  highPulse = !highPulse;
  digitalWrite(led_pin, highPulse);              // toogle LED
} // void HeartBeat

#endif
