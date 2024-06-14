void MqttInit(){
  // configure MQTT client
  mqtt_client.setServer(IPAddress(config.mqtt_broker_addr[0], config.mqtt_broker_addr[1],
                                  config.mqtt_broker_addr[2], config.mqtt_broker_addr[3]),
                        config.mqtt_broker_port.toInt());
  mqtt_client.setCallback(mqtt_callback);   // define Handler for incoming messages
  mqttLastConnectAttempt = 0;
}

void MqttLoop(){
  // If you do not use a MQTT broker so configure the address 0.0.0.0
  if (config.mqtt_broker_addr[0] + config.mqtt_broker_addr[1] + config.mqtt_broker_addr[2] + config.mqtt_broker_addr[3]) {
    // establish connection to MQTT broker
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqtt_client.connected()) {
        // calculate time since last connection attempt
        long now = millis();
        // possible values of mqttLastReconnectAttempt:
        // 0  => never attempted to connect
        // >0 => at least one connect attempt was made
        if ((mqttLastConnectAttempt == 0) || (now - mqttLastConnectAttempt > MQTT_Reconnect_Interval)) {
          mqttLastConnectAttempt = now;
          // attempt to connect
          mqtt_connect();
        }
      } else {
        // client is connected, call the mqtt loop
        mqtt_client.loop();
      }
    }
  }
}

//####################################################################
// connect function for MQTT broker
// called from the main loop
//####################################################################
boolean mqtt_connect() {
  const char* client_id = config.mqtt_broker_client_id.c_str();
  const char* username = config.mqtt_broker_username.c_str();
  const char* password = config.mqtt_broker_password.c_str();
  String willTopic = "tele/" + config.mqtt_devicetopic + "/LWT"; // connect with included "Last-Will-and-Testament" message
  uint8_t willQos = 0;
  boolean willRetain = true;
  const char* willMessage = "Offline";           // LWT message says "Offline"
  String subscribeString = "cmd/" + config.mqtt_devicetopic + "/#";

  WriteLog("[INFO] - trying to connect to MQTT broker", true);
  // try to connect to MQTT
  if (mqtt_client.connect(client_id, username, password, willTopic.c_str(), willQos, willRetain, willMessage )) {
    WriteLog("[INFO] - MQTT connect success", true);
    // subscribe the needed topics
    mqtt_client.subscribe(subscribeString.c_str());
    // publish telemetry message "we are online now"
    mqtt_client.publish(willTopic.c_str(), "Online", true);
  } else {
    WriteLog("[ERR ] - MQTT connect failed, rc =" + (String)mqtt_client.state(), true);
  }
  return mqtt_client.connected();
} // boolean mqtt_connect

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// MQTT functions group
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//####################################################################
// Callback for incoming MQTT messages
//####################################################################
void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  if (debug_mqtt) {
    Serial.printf("mqtt in: %s - ", topic);
    for (uint16_t i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  }

  // extract channel id from topic name
  int channel = 999;
  char * token = strtok(topic, "/");  // initialize token
  token = strtok(NULL, "/");          // now token = 2nd token
  token = strtok(NULL, "/");          // now token = 3rd token, "shutter" or so
  if (debug_mqtt) Serial.printf("command token: %s\n", token);
  if (strncmp(token, "shutter", 7) == 0) {
    token = strtok(NULL, "/");
    if (token != NULL) {
      channel = atoi(token);
    }
  } else if (strncmp(token, "sendconfig", 10) == 0) {
    WriteLog("[INFO] - incoming MQTT command: sendconfig", true);
    mqtt_send_config();
    return;
  } else {
    WriteLog("[ERR ] - incoming MQTT command unknown: " + (String)token, true);
    return;
  }

  // convert payload in string
  payload[length] = '\0';
  String cmdStr = String((char*)payload);

  // print serial message
  WriteLog("[INFO] - incoming MQTT command: channel " + (String)channel + ":", false);
  WriteLog(cmdStr, true);

  if (channel < MAX_CHANNELS) {
    ShutterCmd cmd = string2ShutterCmd(cmdStr.c_str());
    if (cmd != CMD_IDLE) {
      sendCmd(cmd, channel);
    } else {
      WriteLog("[ERR ] - incoming MQTT payload unknown.", true);
    }
  } else {
    WriteLog("[ERR ] - invalid channel, choose one of 0-" + (String)(MAX_CHANNELS - 1), true);
  }
} // void mqtt_callback

//####################################################################
// send devicecounter as mqtt state topic
//####################################################################
void devcnt_handler() {
  if (mqtt_client.connected()) {
    String Topic = "stat/" + config.mqtt_devicetopic + "/devicecounter";
    const char * msg = Topic.c_str();

    uint16_t devCnt;
    EEPROM.get(cntadr, devCnt);

    char devcntstr[10];
    itoa(devCnt, devcntstr, 10);
    mqtt_client.publish(msg, devcntstr, true);
  }
}

//####################################################################
// send status via mqtt
//####################################################################
void mqtt_send_percent_closed_state(int channelNum, int percent, String command) {
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  if (mqtt_client.connected()) {
    char percentstr[4];
    itoa(percent, percentstr, 10);
    String Topic = "stat/" + config.mqtt_devicetopic + "/shutter/" + (String)channelNum;
    const char * msg = Topic.c_str();
    mqtt_client.publish(msg, percentstr);
  }
  WriteLog("[INFO] - command " + command + " for channel " + (String)channelNum + " (" + config.channel_name[channelNum] + ") sent.", true);
} // void mqtt_send_percent_closed_state

//####################################################################
// send config via mqtt
//####################################################################
void mqtt_send_config() {
  String Payload;
  int configCnt = 0, lineCnt = 0;
  char numBuffer[25];

  if (mqtt_client.connected()) {
    // send config of the shutter channels
    for (int channelNum = 0; channelNum < MAX_CHANNELS; channelNum++) {
      if (config.channel_name[channelNum] != "") {
        if (lineCnt == 0) {
          Payload = "{\"channel\":[";
        } else {
          Payload += ", ";
        }
        uint64_t serial;
        EEPROM.get(adresses[channelNum], serial);
        sprintf(numBuffer, "0x%08llx", serial);
        Payload += "{\"id\":" + String(channelNum) + ", \"name\":\"" + config.channel_name[channelNum] + "\", "
                   + "\"serial\":\"" + numBuffer +  "\"}";
        lineCnt++;

        if (lineCnt >= 4) {
          Payload += "]}";
          mqtt_send_config_line(configCnt, Payload);
          lineCnt = 0;
        }
      } // if (config.channel_name[channelNum] != "")
    } // for

    // handle last item
    if (lineCnt > 0) {
      Payload += "]}";
      mqtt_send_config_line(configCnt, Payload);
    }

    // send most important other config info
    uint16_t devCnt;
    EEPROM.get(cntadr, devCnt);
    snprintf(numBuffer, 15, "%d", devCnt);
    Payload = "{\"serialprefix\":\"" + config.serial + "\", "
              + "\"mqtt-clientid\":\"" + config.mqtt_broker_client_id + "\", "
              + "\"mqtt-devicetopic\":\"" + config.mqtt_devicetopic + "\", "
              + "\"devicecounter\":" + (String)numBuffer + ", "
              + "\"new_learn_mode\":" + (String)config.learn_mode + ", ";
              + "\"shade_support\":"  + (String)config.shade_support + "}";
              + "\"shade_enable\":"   + (String)config.shade_enable + "}";
    mqtt_send_config_line(configCnt, Payload);
  } // if (mqtt_client.connected())
} // void mqtt_send_config

//####################################################################
// send one config telegram via mqtt
//####################################################################
void mqtt_send_config_line(int & counter, String Payload) {
  String Topic = "stat/" + config.mqtt_devicetopic + "/config/" + (String)counter;
  if (debug_mqtt) Serial.println("mqtt send: " + Topic + " - " + Payload);
  mqtt_client.publish(Topic.c_str(), Payload.c_str());
  counter++;
  yield();
} // void mqtt_send_config_line
