Schedules scheduler;

Schedules* getScheduler() {
  return &scheduler;
}

String lastTime;
String nextTime;

void InitSchedules() {
  int index = 0;
  if (DEVICE)
    scheduler.FileName = "/schedules2.txt";
  else
    scheduler.FileName = "/schedules.txt";
  if (!SPIFFS.exists(scheduler.FileName))
    WriteLog("[INFO] - No schedules found", true);
  File myFile = SPIFFS.open(scheduler.FileName, "r");
  while (myFile.available()) {
    String lineString = myFile.readStringUntil('\n');
    scheduler.Items[index].FromString(lineString);
    index++;
  }
  myFile.close();
}

void SchedulerLoop() {
  //only start when time is ready:
  if (!timeIsSet)
    return;
  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm * timeinfo = localtime(&now);
  uint8_t weekday = timeinfo->tm_wday - 1; //start by 0 // days since Sunday 0-6
  if(weekday > 6)
    weekday = 6;
  uint8_t hour = timeinfo->tm_hour;
  uint8_t minute = timeinfo->tm_min;

  if (scheduler.isUpdated) {
    scheduler.isUpdated = false;
    nextTime = "";
  }
  //Nothing found or wait for next day
  if (nextTime.length() <= 0) {
    nextTime = scheduler.GetNextScheduledTime(weekday, hour, minute);
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

  for ( uint8_t i = 0; i < scheduler.length(); i++) {
    if (scheduler.Items[i].schedule != i)
      break;

    if (!scheduler.Items[i].isInWeekday(weekday))
      continue;

    if (!(scheduler.Items[i].getHour() == hour && scheduler.Items[i].getMinute() == minute))
      continue;
    uint8_t channels[MAX_CHANNELS];
    uint8_t channelCount = scheduler.Items[i].getChannels(channels);

    for ( uint8_t c = 0; c < channelCount; c++)
      sendCmd(string2ShutterCmd(scheduler.Items[i].typeText.c_str()), channels[c]);
  }
  digitalWrite(led_pin, HIGH);
  WriteLog("[INFO] - Schedule Executed. Runtime: " + String(millis() - startTime) + "ms" , true);
}
