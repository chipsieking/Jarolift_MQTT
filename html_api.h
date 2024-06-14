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

#ifndef HTML_API_H
#define HTML_API_H


//####################################################################
// API call to get data or execute commands via WebIf
//####################################################################
void html_api() {
  if (debug_webui) {
    Serial.printf("html_api server.args()=%d\n", server.args());
  }
  if (server.args() > 0 ) {
    // get server args from HTML POST
    String    cmd = "";
    unsigned  channel = MAX_CHANNELS;  // needs to be overridden to become valid
    String    param = "";
    bool      checked = false;
    for (unsigned i = 0; i < server.args(); ++i) {
      if (debug_webui) {
        Serial.printf("server.argName(%d): '%s'\n", i, server.argName(i).c_str());
        Serial.printf(" urldecode: '%s'\n", urldecode(server.arg(i)).c_str());
      }
      if (server.argName(i) == "cmd")     cmd     = urldecode(server.arg(i));
      if (server.argName(i) == "channel") channel = server.arg(i).toInt();

      if (server.argName(i) == "param")   param   = urldecode(server.arg(i));
      if (server.argName(i) == "checked") checked = (urldecode(server.arg(i)) == "true");

      if (server.argName(i) == "ssid")      config.ssid     = urldecode(server.arg(i));
      if (server.argName(i) == "password")  config.password = urldecode(server.arg(i));

      if (server.argName(i) == "mqtt_broker_port")      config.mqtt_broker_port       = urldecode(server.arg(i));
      if (server.argName(i) == "mqtt_broker_client_id") config.mqtt_broker_client_id  = urldecode(server.arg(i));
      if (server.argName(i) == "mqtt_broker_username")  config.mqtt_broker_username   = urldecode(server.arg(i));
      if (server.argName(i) == "mqtt_broker_password")  config.mqtt_broker_password   = urldecode(server.arg(i));
      if (server.argName(i) == "mqtt_devicetopic")      config.mqtt_devicetopic_new   = urldecode(server.arg(i));

      if (server.argName(i) == "master_msb_str") config.master_msb_str = urldecode(server.arg(i));
      if (server.argName(i) == "master_lsb_str") config.master_lsb_str = urldecode(server.arg(i));

      if (server.argName(i) == "ip_0") if (checkRange(server.arg(i)))   config.ip[0] =  server.arg(i).toInt();
      if (server.argName(i) == "ip_1") if (checkRange(server.arg(i)))   config.ip[1] =  server.arg(i).toInt();
      if (server.argName(i) == "ip_2") if (checkRange(server.arg(i)))   config.ip[2] =  server.arg(i).toInt();
      if (server.argName(i) == "ip_3") if (checkRange(server.arg(i)))   config.ip[3] =  server.arg(i).toInt();
      if (server.argName(i) == "nm_0") if (checkRange(server.arg(i)))   config.netmask[0] =  server.arg(i).toInt();
      if (server.argName(i) == "nm_1") if (checkRange(server.arg(i)))   config.netmask[1] =  server.arg(i).toInt();
      if (server.argName(i) == "nm_2") if (checkRange(server.arg(i)))   config.netmask[2] =  server.arg(i).toInt();
      if (server.argName(i) == "nm_3") if (checkRange(server.arg(i)))   config.netmask[3] =  server.arg(i).toInt();
      if (server.argName(i) == "gw_0") if (checkRange(server.arg(i)))   config.gateway[0] =  server.arg(i).toInt();
      if (server.argName(i) == "gw_1") if (checkRange(server.arg(i)))   config.gateway[1] =  server.arg(i).toInt();
      if (server.argName(i) == "gw_2") if (checkRange(server.arg(i)))   config.gateway[2] =  server.arg(i).toInt();
      if (server.argName(i) == "gw_3") if (checkRange(server.arg(i)))   config.gateway[3] =  server.arg(i).toInt();

      if (server.argName(i) == "mqtt_broker_addr_0") if (checkRange(server.arg(i)))   config.mqtt_broker_addr[0] =  server.arg(i).toInt();
      if (server.argName(i) == "mqtt_broker_addr_1") if (checkRange(server.arg(i)))   config.mqtt_broker_addr[1] =  server.arg(i).toInt();
      if (server.argName(i) == "mqtt_broker_addr_2") if (checkRange(server.arg(i)))   config.mqtt_broker_addr[2] =  server.arg(i).toInt();
      if (server.argName(i) == "mqtt_broker_addr_3") if (checkRange(server.arg(i)))   config.mqtt_broker_addr[3] =  server.arg(i).toInt();

      if (server.argName(i) == "dhcp")
        config.dhcp = (urldecode(server.arg(i)) == "true");
      if (server.argName(i) == "learn_mode")
        config.learn_mode = (urldecode(server.arg(i)) == "true");
      if (server.argName(i) == "shade_support")
        config.shade_support = (urldecode(server.arg(i)) == "true");
      if (server.argName(i) == "shade_enable")
        config.shade_enable  = (urldecode(server.arg(i)) == "true");
      if (server.argName(i) == "set_and_generate_serial")
        config.set_and_generate_serial = (urldecode(server.arg(i)) == "true");
      if (server.argName(i) == "serial")
        config.new_serial = urldecode(server.arg(i));
      if (server.argName(i) == "set_devicecounter")
        config.set_devicecounter = (urldecode(server.arg(i)) == "true");
      if (server.argName(i) == "devicecounter")
        config.new_devicecounter = urldecode(server.arg(i));
    } // for
    if (cmd != "") {
      if (cmd == "eventlog") {
        String values = "";
        // web_log_message[] is an array of strings, used as a circular buffer
        // web_log_message_nextfree points to the next free-to-use buffer line
        // first message in log depends of web_log_message_rotated
        //   false: first message is entry 0, last is (web_log_message_nextfree-1)
        //   true:  first/oldest message is (web_log_message_nextfree), last is (web_log_message_nextfree-1)
        if (web_log_message_rotated) {
          int msg_end = web_log_message_nextfree + NUM_WEB_LOG_MESSAGES;
          for (int msg_ptr = web_log_message_nextfree; msg_ptr < msg_end; msg_ptr++) {
            values += web_log_message[(msg_ptr % NUM_WEB_LOG_MESSAGES)] + "\n";
          }
        } else {
          for (int msg_ptr = 0; msg_ptr < web_log_message_nextfree; msg_ptr++) {
            values += web_log_message[msg_ptr] + "\n";
          }
        }
        server.send(200, "text/plain", values);
      } else if (cmd == "save") {
        // handled in main loop
        web_cmd = cmd;
      } else if (cmd == "restart") {
        // handled in main loop
        web_cmd = cmd;
      } else if (cmd == "get shutter ctrl") {
        String values = "";
        for (unsigned i = 0; i < MAX_CHANNELS; ++i) {
          values += "chn="    + String(i) + ";";
          values += "name="   + config.channel_name[i] + ";";
          values += "upDown=" + String((config.flags[i] & CHANNEL_FLAG_AUTO_UPDOWN) != 0) + ";";
          values += "shade="  + String((config.flags[i] & CHANNEL_FLAG_AUTO_SHADE)  != 0) + "\n";
        }
        if (debug_webui) {
          Serial.printf("'%s'\n", values.c_str());
        }
        server.send(200, "text/plain", values);
      } else if (cmd == "get shutter cfg") {
        String values = "";
        for (unsigned i = 0; i < MAX_CHANNELS; ++i) {
          values += "chn="  + String(i) + ";";
          values += "name=" + config.channel_name[i] + ";";
          // shutter runtime cfg for shading is in [ms], change to [1/10s] for proper web display
          values += "time=" + String(config.shade_runtime[i] / 100) + "\n";
        }
        if (debug_webui) {
          Serial.printf("'%s'\n", values.c_str());
        }
        server.send(200, "text/plain", values);
      } else if (cmd == "get config") {
        uint16_t devCnt;
        EEPROM.get(cntadr, devCnt);
        String values = "";
        values += "ssid=" + config.ssid + "\n";
        values += "password=" + config.password + "\n";
        if (config.dhcp) {
          values += "checkbox=dhcp=1\n";
        } else {
          values += "checkbox=dhcp=0\n";
        }
        values += "ip_0=" + (String) config.ip[0] + "\n";
        values += "ip_1=" + (String) config.ip[1] + "\n";
        values += "ip_2=" + (String) config.ip[2] + "\n";
        values += "ip_3=" + (String) config.ip[3] + "\n";
        values += "nm_0=" + (String) config.netmask[0] + "\n";
        values += "nm_1=" + (String) config.netmask[1] + "\n";
        values += "nm_2=" + (String) config.netmask[2] + "\n";
        values += "nm_3=" + (String) config.netmask[3] + "\n";
        values += "gw_0=" + (String) config.gateway[0] + "\n";
        values += "gw_1=" + (String) config.gateway[1] + "\n";
        values += "gw_2=" + (String) config.gateway[2] + "\n";
        values += "gw_3=" + (String) config.gateway[3] + "\n";
        values += "mqtt_broker_addr_0=" + (String) config.mqtt_broker_addr[0] + "\n";
        values += "mqtt_broker_addr_1=" + (String) config.mqtt_broker_addr[1] + "\n";
        values += "mqtt_broker_addr_2=" + (String) config.mqtt_broker_addr[2] + "\n";
        values += "mqtt_broker_addr_3=" + (String) config.mqtt_broker_addr[3] + "\n";
        values += "mqtt_broker_port=" + config.mqtt_broker_port + "\n";
        values += "mqtt_broker_username=" + config.mqtt_broker_username + "\n";
        values += "mqtt_broker_password=" + config.mqtt_broker_password + "\n";
        values += "mqtt_broker_client_id=" + config.mqtt_broker_client_id + "\n";
        values += "mqtt_devicetopic=" + config.mqtt_devicetopic + "\n";
        values += "master_msb_str=" + config.master_msb_str + "\n";
        values += "master_lsb_str=" + config.master_lsb_str + "\n";
        if (config.learn_mode) {
          values += "checkbox=learn_mode=1\n";
        } else {
          values += "checkbox=learn_mode=0\n";
        }
        if (config.shade_support) {
          values += "checkbox=shade_support=1\n";
        } else {
          values += "checkbox=shade_support=0\n";
        }
        if (config.shade_enable) {
          values += "checkbox=shade_enable=1\n";
        } else {
          values += "checkbox=shade_enable=0\n";
        }
        values += "serial=" + config.serial + "\n";
        values += "checkbox=set_and_generate_serial=0\n";
        values += "devicecounter=" + (String)devCnt + "\n";
        values += "checkbox=set_devicecounter=0\n";
        values += "text=versionstr=Firmware " + String(PROGRAM_VERSION) + "\n";
        server.send(200, "text/plain", values);
      } else if (cmd == "set channel name") {
        if (channel < MAX_CHANNELS) {
          config.channel_name[channel] = param;
          WriteConfig();
          String status_text = "Updating channel " + String(channel) + " name to '" + param + "'.";
          server.send(200, "text/plain", status_text);
        }
      } else if (cmd == "set shade runtime") {
        if (channel < MAX_CHANNELS) {
          // use param for carrying runtime, runtime is in [1/10s] -> normalize to [ms]
          config.shade_runtime[channel] = param.toInt() * 100;
          WriteConfigShadeRuntime(channel);
          String status_text = "Updating channel " + String(channel) + " shade runtime to '" + String(config.shade_runtime[channel]) + "'.";
          server.send(200, "text/plain", status_text);
        }
      } else if (cmd == "checkAutoUpdown") {
        if (channel < MAX_CHANNELS) {
          if (checked) {
            config.flags[channel] |= CHANNEL_FLAG_AUTO_UPDOWN;
          } else {
            config.flags[channel] &= CHANNEL_FLAG_AUTO_UPDOWN;
          }
          WriteChannelFlags(channel);
          String status_text = "Updating channel " + String(channel) + " auto up/down to '" + checked + "'.";
          server.send(200, "text/plain", status_text);
        }
      } else if (cmd == "checkAutoShade") {
        if (channel < MAX_CHANNELS) {
          if (checked) {
            config.flags[channel] |= CHANNEL_FLAG_AUTO_SHADE;
          } else {
            config.flags[channel] &= CHANNEL_FLAG_AUTO_SHADE;
          }
          WriteChannelFlags(channel);
          String status_text = "Updating channel " + String(channel) + " auto shade to '" + checked + "'.";
          server.send(200, "text/plain", status_text);
        }
      } else {
        if (channel < MAX_CHANNELS) {
          web_cmd_channel = channel;
          web_cmd = cmd;
          String status_text = "Running command '" + cmd + "' for channel " + channel + ".";
          server.send(200, "text/plain", status_text);
        }
      }

    } else {
      String status_text = "No command to execute!";
      server.send(200, "text/plain", status_text);
    }
    delay(100);
  }
} // void html_api

#endif // HTML_API_H
