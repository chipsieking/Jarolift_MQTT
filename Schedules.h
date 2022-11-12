#ifndef Schedules_h
#define Schedules_h

class Schedule {
  public:
    uint8_t schedule = 0xff;
    uint8_t weekdays;
    String timeText;
    String typeText;
    uint8_t channels;
    String toString() {
      String str = "schedule=";
      str.concat(schedule);
      str.concat("&weekday=");
      str.concat(weekdays);
      str.concat("&time=");
      str.concat(timeText);
      str.concat("&type=");
      str.concat(typeText);
      str.concat("&channel=");
      str.concat(channels);
      return str;
    }
    uint8_t getHour() {
      uint8_t index = timeText.indexOf(':');
      String hourTxt = timeText.substring(0, index + 1);
      return hourTxt.toInt();
    }
    uint8_t getMinute() {
      uint8_t index = timeText.indexOf(':');
      String minTxt = timeText.substring(index + 1);
      return minTxt.toInt();
    }
    bool isInWeekday(uint8_t weekday) {
      return ((1 << weekday) & weekdays) == 1 << weekday;
    }
    uint8_t getChannels(uint8_t *channelList) {
      uint8_t i = 0;
      for ( uint8_t c = 0; c < 16; c++)
        if (((1 << c) & channels) == 1 << c)
          channelList[i++] = c;
      channelList[i] = '\0';
      return i;
    }
    void FromString(String line) {
      //schedule=0&weekday=31&time=09:43&type=up&channel=23
      for (uint8_t i = 0; i < 5; i++) {
        String str = getValue(line, '&', i);
        String name = getValue(str, '=', 0);
        String val = getValue(str, '=', 1);       
        if (name == "schedule")
          schedule = val.toInt();
        if (name == "weekday")
          weekdays = val.toInt();
        if (name == "time")
          timeText = val;
        if (name == "type")
          typeText = val;
        if (name == "channel")
          channels = val.toInt();
      }
    }
    private:
    String getValue(String data, char separator, int index)
    {
      int found = 0;
      int strIndex[] = { 0, -1 };
      int maxIndex = data.length() - 1;

      for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
          found++;
          strIndex[0] = strIndex[1] + 1;
          strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
      }
      return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
    }
};

class Schedules {
#define NoOfItems 10
  public:
    Schedule Items[NoOfItems];
    bool isUpdated = false;
    String FileName = "/schedules.txt";
    void Print() {
      for ( uint8_t i = 0; i < NoOfItems; i++) {
        if (Items[i].schedule != i)
          break;
        Serial.println(Items[i].toString());
      }
    }
    uint8_t length() {
      return NoOfItems;
    }
    void Clear() {
      for ( uint8_t i = 0; i < NoOfItems; i++)
        Items[i].schedule = 0xff;
    }
    String GetNextScheduledTime(uint8_t weekday, uint8_t hour, uint8_t minute) {
      uint8_t hourMin = 24;
      uint8_t minuteMin = 60;
      uint8_t entry = 0xff;
      for ( uint8_t i = 0; i < length(); i++) {
        if (Items[i].schedule != i)
          break;

        if (!Items[i].isInWeekday(weekday))
          continue;
        uint8_t hourCurr = Items[i].getHour();
        uint8_t minuteCurr = Items[i].getMinute();

        if (hourCurr < hour || (hourCurr == hour && minuteCurr < minute))
          continue;

        if (hourCurr < hourMin) {
          hourMin = hourCurr;
          minuteMin = minuteCurr;
          entry = i;
          continue;
        }
        if (hourCurr == hourMin)
          if (minuteCurr < minuteMin) {
            minuteMin = minuteCurr;
            entry = i;
          }
      }
      if(entry == 0xff)
        return "";
      return Items[entry].timeText;
    }
};
#endif
