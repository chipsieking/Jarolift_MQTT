#include "Schedule.h"
Schedules Schedules;

String lastTime;
String nextTime;

void InitSchedules() {
  int index = 0;
  if (DEVICE)
    Schedules.FileName = "/schedules2.txt";
  else
    Schedules.FileName = "/schedules.txt";
  if (!SPIFFS.exists(Schedules.FileName))
    WriteLog("[INFO] - No schedules found", true);
  File myFile = SPIFFS.open(Schedules.FileName, "r");
  while (myFile.available()) {
    String lineString = myFile.readStringUntil('\n');
    Schedules.Items[index].FromString(lineString);
    index++;
  }
  myFile.close();

  SetSchedulerPointer(&Schedules);
}

void SchedulerLoop() {
  //only start when time is ready:
  if (time_is_set_first)
    return;
  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm * timeinfo = localtime(&now);
  uint8_t weekday = timeinfo->tm_wday - 1; //start by 0 // days since Sunday 0-6
  if(weekday > 6)
    weekday = 6;
  uint8_t hour = timeinfo->tm_hour;
  uint8_t minute = timeinfo->tm_min;

  if (Schedules.isUpdated) {
    Schedules.isUpdated = false;
    nextTime = "";
  }
  //Nothing found or wait for next day
  if (nextTime.length() <= 0) {
    nextTime = Schedules.GetNextScheduledTime(weekday, hour, minute);
    if (nextTime.length() > 0 && nextTime != lastTime)
      WriteLog("[INFO] - Next Schedule: Day " + String(weekday) + " Time " + nextTime, true);
  }
  if (nextTime.length() == 0)
    return;

  //Already executed
  if (nextTime == lastTime) {
    nextTime = "";
    return;
  }

  uint8_t index = nextTime.indexOf(':');
  String hourTxt = nextTime.substring(0, index + 1);
  uint8_t hourInt = hourTxt.toInt();
  String minTxt = nextTime.substring(index + 1);
  uint8_t minInt = minTxt.toInt();
  if (hour == hourInt && minute == minInt) {
    ExecuteSchedule(weekday, hour, minute);
    lastTime = nextTime;
    nextTime = "";
  }
}

void ExecuteSchedule(uint8_t weekday, uint8_t hour, uint8_t minute) {
  long startTime = millis();
  digitalWrite(led_pin, LOW);

  for ( uint8_t i = 0; i < Schedules.length(); i++) {
    if (Schedules.Items[i].schedule != i)
      break;

    if (!Schedules.Items[i].isInWeekday(weekday))
      continue;

    if (!(Schedules.Items[i].getHour() == hour && Schedules.Items[i].getMinute() == minute))
      continue;
    uint8_t channels[16];
    uint8_t channelCount = Schedules.Items[i].getChannels(channels);

    for ( uint8_t c = 0; c < channelCount; c++)
      SendCommand(channels[c], Schedules.Items[i].typeText);
  }
  digitalWrite(led_pin, HIGH);
  WriteLog("[INFO] - Schedule Executed. Runtime: " + String(millis() - startTime) + "ms" , true);
}

void SendCommand(uint8_t channel, String cmd) {
  detachInterrupt(RX_PORT); // Interrupt on change of RX_PORT
  delay(5);

  if (cmd == "up")
    cmd_up(channel);
  else if (cmd == "down")
    cmd_down(channel);
  else if (cmd == "stop")
    cmd_stop(channel);
  else if (cmd == "shade")
    cmd_shade(channel);

  cc1101.cmdStrobe(CC1101_SCAL);
//  delay(50);
//  enterrx();
//  delay(200);
//  attachInterrupt(RX_PORT, radio_rx_measure, CHANGE); // Interrupt on change of RX_PORT
//  delay(50);
//  CheckRxBuffer();
  delay(50);
  enterrx();
  delay(200);
  attachInterrupt(RX_PORT, radio_rx_measure, CHANGE); // Interrupt on change of RX_PORT
  delay(200);
  CheckRxBuffer();
  delay(500);
  iset = false;
}
