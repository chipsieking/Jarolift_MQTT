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

#ifndef HELPERS_H
#define HELPERS_H


//####################################################################
// Check if the Value is between 0-255
//####################################################################
inline boolean checkRange(String str) {
  return ((str.toInt() >= 0) && (str.toInt() <= 255));
}

//####################################################################
void WriteStringToEEPROM(unsigned addr, String str) {
  char buf[str.length() + 1];
  str.toCharArray(buf, sizeof(buf));
  for (unsigned i = 0; i < str.length(); ++i) {
    // do not store trailing \0 in EEPROM
    EEPROM.write(addr + i, buf[i]);
  }
}

//####################################################################
String ReadStringFromEEPROM(unsigned addr, unsigned maxLen) {
  unsigned i = 0;
  char chr;
  String str = "";
  while (i < maxLen) {
    chr = EEPROM.read(addr + i++);
    if (chr == 0) break;
    str.concat(chr);
  }
  return str;
}

//####################################################################
void EEPROMWritelong(int address, long value)
{
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  // Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

//####################################################################
long EEPROMReadlong(long address)
{
  // Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  // Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

//####################################################################
// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
//####################################################################
unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

//####################################################################
String urldecode(String input) { // (based on https://code.google.com/p/avr-netino/)
  char c;
  String str = "";

  for (unsigned i = 0; i < input.length(); ++i) {
    c = input[i];
    if (c == '+') c = ' ';
    if (c == '%') {
      i++;
      c = input[i];
      i++;
      c = (h2int(c) << 4) | h2int(input[i]);
    }
    str.concat(c);
  }
  return str;
}

#endif
