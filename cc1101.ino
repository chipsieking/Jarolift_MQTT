#include "cc1101.h"
#include <KeeloqLib.h>

// User configuration
#define TX_PORT             4       // output port for transmission
#define RX_PORT             5       // input port for reception

#define PULSE_LEN_SHORT     400     // pulse-width in microseconds. Adapt for your use...
#define PULSE_LEN_LONG      800

CC1101 cc1101;                      // CC1101 hardware driver
volatile bool iset = false;

// forward declarations
void enterTx();
void enterRx();

byte disc_low[MAX_CHANNELS]   = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0,  0x0,  0x0};
byte disc_high[MAX_CHANNELS]  = {0x0, 0x0, 0x0, 0x0, 0x0,  0x0,  0x0,  0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

char rx_serial_array[8]  = {0};
char rx_disc_low[8]      = {0};
char rx_disc_high[8]     = {0};
byte rx_function         = 0;



// RX variables and defines
#define debounce         200              // Ignoring short pulses in reception... no clue if required and if it makes sense ;)
#define pufsize          216              // Pulsepuffer
uint32_t rx_serial       = 0;
uint32_t rx_hopcode      = 0;
uint16_t rx_disc_h       = 0;
int rx_device_key_msb    = 0x0;           // stores cryptkey MSB
int rx_device_key_lsb    = 0x0;           // stores cryptkey LSB
volatile uint32_t decoded         = 0x0;  // decoded hop code
volatile byte pbwrite;
volatile unsigned int lowbuf[pufsize];    // ring buffer storing LOW pulse lengths
volatile unsigned int hibuf[pufsize];     // ring buffer storing HIGH pulse lengths
volatile byte value = 0;                  // Stores RSSI Value


void IRAM_ATTR rxIrq();

void CC1101_init() {
  // initialize the transceiver chip
  WriteLog("[INFO] - initializing the CC1101 Transceiver. If you get stuck here, it is probably not connected.", true);
  cc1101.init();
  cc1101.setSyncWord(199, false);   // ??? TODO magic sync word
  cc1101.setCarrierFreq(CFREQ_433);
  cc1101.disableAddressCheck();   // if not specified, will only display "packet received"

  pinMode(TX_PORT, OUTPUT);                   // CC1101 TX pin
  pinMode(RX_PORT, INPUT_PULLUP);             // CC1101 RX pin
  attachInterrupt(RX_PORT, rxIrq, CHANGE);    // interrupt on change of RX_PORT
}

void CC1101_loop() {
  if (iset) {
    cc1101.cmdStrobe(CC1101_SCAL);
    delay(50);
    enterRx();
    iset = false;
    delay(200);
    attachInterrupt(RX_PORT, rxIrq, CHANGE);  // interrupt on change of RX_PORT
  }
  checkRxBuffer();

  iset = true;
  detachInterrupt(RX_PORT);                   // no interrupt on change of RX_PORT
  delay(1);
}

void checkRxBuffer() {
  static int steadycnt;

  // Check if RX buffer is full
  if ((lowbuf[0] > 3650) && (lowbuf[0] < 4300) && (pbwrite >= 65) && (pbwrite <= 75)) {     // Decode received data...
    if (debug_log_radio_receive_all)
      WriteLog("[INFO] - received data", true);
    iset = true;
    ReadRSSI();
    pbwrite = 0;

    for (int i = 0; i <= 31; i++) {                          // extracting Hopcode
      if (lowbuf[i + 1] < hibuf[i + 1]) {
        rx_hopcode = (rx_hopcode & ~(1 << i)) | (0 << i);
      } else {
        rx_hopcode = (rx_hopcode & ~(1 << i)) | (1 << i);
      }
    }
    for (int i = 0; i <= 27; i++) {                         // extracting Serialnumber
      if (lowbuf[i + 33] < hibuf[i + 33]) {
        rx_serial = (rx_serial & ~(1 << i)) | (0 << i);
      } else {
        rx_serial = (rx_serial & ~(1 << i)) | (1 << i);
      }
    }
    rx_serial_array[0] = (rx_serial >> 24) & 0xFF;
    rx_serial_array[1] = (rx_serial >> 16) & 0xFF;
    rx_serial_array[2] = (rx_serial >> 8) & 0xFF;
    rx_serial_array[3] = rx_serial & 0xFF;

    for (int i = 0; i <= 3; i++) {                        // extracting function code
      if (lowbuf[61 + i] < hibuf[61 + i]) {
        rx_function = (rx_function & ~(1 << i)) | (0 << i);
      } else {
        rx_function = (rx_function & ~(1 << i)) | (1 << i);
      }
    }

    for (int i = 0; i <= 7; i++) {                        // extracting high disc
      if (lowbuf[65 + i] < hibuf[65 + i]) {
        rx_disc_h = (rx_disc_h & ~(1 << i)) | (0 << i);
      } else {
        rx_disc_h = (rx_disc_h & ~(1 << i)) | (1 << i);
      }
    }

    rx_disc_high[0] = rx_disc_h & 0xFF;
    rx_keygen ();
    rx_decoder();
    if (rx_function == 0x4)steadycnt++;           // to detect a long press....
    else steadycnt--;
    if (steadycnt > 10 && steadycnt <= 40) {
      rx_function = 0x3;
      steadycnt = 0;
    }

    Serial.printf(" serialnumber: 0x%08x // function code: 0x%02x // disc: 0x%02x\n\n", rx_serial, rx_function, rx_disc_h);

    // send mqtt message with received Data:
    if (mqtt_client.connected() && mqtt_send_radio_receive_all) {
      String Topic = "stat/" + config.mqtt_devicetopic + "/received";
      const char * msg = Topic.c_str();
      char payload[220];
      snprintf(payload, sizeof(payload),
               "{\"serial\":\"0x%08x\", \"rx_function\":\"0x%x\", \"rx_disc_low\":%d, \"rx_disc_high\":%d, \"RSSI\":%d, \"counter\":%d, \"rx_device_key_lsb\":\"0x%08x\", \"rx_device_key_msb\":\"0x%08x\", \"decoded\":\"0x%08x\"}",
               rx_serial, rx_function, rx_disc_low[0], rx_disc_h, value, rx_disc_low[3], rx_device_key_lsb, rx_device_key_msb, decoded );
      mqtt_client.publish(msg, payload);
    }

    rx_disc_h = 0;
    rx_hopcode = 0;
    rx_function = 0;
  }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CC1101 radio functions group
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//####################################################################
// Receive Routine
//####################################################################
void IRAM_ATTR rxIrq() {
  static long LineUp, LineDown, Timeout;
  long LowVal, HighVal;
  int pinstate = digitalRead(RX_PORT); // Read current pin state
  if (micros() - Timeout > 3500) {
    pbwrite = 0;
  }
  if (pinstate) {
    // pin is now HIGH, was low
    LineUp = micros();                // Get actual time in LineUp
    LowVal = LineUp - LineDown;       // calculate the LOW pulse time
    if (LowVal < debounce) return;
    if ((LowVal > 300) && (LowVal < 4300)) {
      if ((LowVal > 3650) && (LowVal < 4300)) {
        Timeout = micros();
        pbwrite = 0;
        lowbuf[pbwrite] = LowVal;
        pbwrite++;
      }
      if ((LowVal > 300) && (LowVal < 1000)) {
        lowbuf[pbwrite] = LowVal;
        pbwrite++;
        Timeout = micros();
      }
    }
  } else {
    LineDown = micros();          // line went LOW after being HIGH
    HighVal = LineDown - LineUp;  // calculate the HIGH pulse time
    if (HighVal < debounce) return;
    if ((HighVal > 300) && (HighVal < 1000)) {
      hibuf[pbwrite] = HighVal;
    }
  }
}

//####################################################################
// Generation of the encrypted message (Hopcode)
//####################################################################
uint32_t keeloq(uint32_t serial, uint32_t disc, uint16_t devCnt) {
  Keeloq k1(config.master_msb_num, config.master_lsb_num);
  int keyLo = k1.decrypt(serial | 0x20000000);
  int keyHi = k1.decrypt(serial | 0x60000000);
  Serial.printf(" created devicekey low: 0x%08x // high: 0x%08x\n", keyLo, keyHi);

  Keeloq k2(keyHi, keyLo);
  uint32_t result = (disc << 16) | devCnt;  // Append counter value to discrimination value
  return k2.encrypt(result);
}

//####################################################################
// Keygen generates the device crypt key in relation to the masterkey and provided serial number.
// Here normal key-generation is used according to 00745a_c.PDF Appendix G.
// https://github.com/hnhkj/documents/blob/master/KEELOQ/docs/AN745/00745a_c.pdf
//####################################################################
// void keygen(uint64_t newSerial) {
//   Keeloq k(config.master_msb_num, config.master_lsb_num);
//   device_key_lsb = k.decrypt(newSerial | 0x20000000);
//   device_key_msb = k.decrypt(newSerial | 0x60000000);
//   // Serial.printf(" created devicekey low: 0x%08x // high: 0x%08x\n", device_key_lsb, device_key_msb);
// } // void keygen

//####################################################################
// Simple bitbanging TX routine.
// cycles:
// - 2: standard number of TX cycles for up/down/stop cmds
// - multiple cycles for continuously pressed buttons
//####################################################################
void CC1101_send(unsigned channel, txCmd_t cmd, int cycles, byte rxFunc) {
  uint32_t serial;
  EEPROM.get(EEPROM_ADDR_SERIAL[channel], serial);
  uint32_t disc = ((uint32_t)disc_low[channel] << 8) | (serial & 0xFF);
  uint16_t devCnt;
  EEPROM.get(EEPROM_ADDR_DEV_CNT, devCnt);
  uint32_t dec = keeloq(serial, disc, devCnt);
  uint64_t pack = ((uint64_t)cmd << 60) | ((uint64_t)serial << 32) | dec;

  rx_function = rxFunc;
  rx_disc_low[0]  = disc_low[channel];
  rx_disc_high[0] = disc_high[channel];


  enterTx();
  for (int a = 0; a < cycles; a++) {
    sendPreamble();

    // send message body
    for (unsigned i = 0; i < 64; i++) { // first 64 bits of message
      sendBit((pack >> i) & 0x1);
    }
    for (unsigned i = 0; i < 8; i++) {  // last 8 bits (motors 9-16)
      sendBit((disc_high[channel] >> i) & 0x1);
    }

    sendDelimiter();
  }
  enterRx();

  rx_serial_array[0] = (serial >> 24) & 0xFF;
  rx_serial_array[1] = (serial >> 16) & 0xFF;
  rx_serial_array[2] = (serial >> 8) & 0xFF;
  rx_serial_array[3] = serial & 0xFF;

  EEPROM.put(EEPROM_ADDR_DEV_CNT, ++devCnt);
}

// send frame preamble
void sendPreamble() {
  digitalWrite(TX_PORT, LOW);       // 1560us "on" pulse -> start of frame
  delayMicroseconds(1560);
  for (int i = 0; i < 12; ++i) {    // generate 12 "off"/"on" cycles with 800us period -> sync
    digitalWrite(TX_PORT, HIGH);
    delayMicroseconds(PULSE_LEN_SHORT);
    digitalWrite(TX_PORT, LOW);
    delayMicroseconds(PULSE_LEN_SHORT);
  }
  digitalWrite(TX_PORT, HIGH);      // 3960us "off" pulse -> start delimiter
  delayMicroseconds(3960);
}

// send single bit in OOK encoding
void sendBit(bool ook) {
  digitalWrite(TX_PORT, LOW);
  delayMicroseconds(ook ? PULSE_LEN_SHORT : PULSE_LEN_LONG);
  digitalWrite(TX_PORT, HIGH);
  delayMicroseconds(ook ? PULSE_LEN_LONG : PULSE_LEN_SHORT);
}

// send delimiter
void sendDelimiter() {
  digitalWrite(TX_PORT, HIGH);      // 16ms "off" pulse -> end delimiter
  delay(16);                        // delay in loop context is save for wdt
}

//####################################################################
// Calculate device code from received serial number
//####################################################################
void rx_keygen() {
  Keeloq k(config.master_msb_num, config.master_lsb_num);
  uint32_t keylow = rx_serial | 0x20000000;
  unsigned long enc = k.decrypt(keylow);
  rx_device_key_lsb  = enc;        // Stores LSB devicekey 16Bit
  keylow = rx_serial | 0x60000000;
  enc    = k.decrypt(keylow);
  rx_device_key_msb  = enc;        // Stores MSB devicekey 16Bit

  Serial.printf(" received devicekey low: 0x%08x // high: 0x%08x", rx_device_key_lsb, rx_device_key_msb);
} // void rx_keygen

//####################################################################
// Decoding of the hopping code
//####################################################################
void rx_decoder() {
  Keeloq k(rx_device_key_msb, rx_device_key_lsb);
  unsigned int result = rx_hopcode;
  decoded = k.decrypt(result);
  rx_disc_low[0] = (decoded >> 24) & 0xFF;
  rx_disc_low[1] = (decoded >> 16) & 0xFF;
  rx_disc_low[2] = (decoded >> 8) & 0xFF;
  rx_disc_low[3] = decoded & 0xFF;

  Serial.printf(" // decoded: 0x%08x\n", decoded);
} // void rx_decoder

//####################################################################
// calculate RSSI value (Received Signal Strength Indicator)
//####################################################################
void ReadRSSI() {
  byte rssi = 0;
  rssi = (cc1101.readReg(CC1101_RSSI, CC1101_STATUS_REGISTER));
  if (rssi >= 128) {
    value = 255 - rssi;
    value /= 2;
    value += 74;
  } else {
    value = rssi / 2;
    value += 74;
  }
  Serial.print(" CC1101_RSSI ");
  Serial.println(value);
} // void ReadRSSI

//####################################################################
// put CC1101 to receive mode
//####################################################################
void enterRx() {
  cc1101.setRxState();
  delay(2);
  long rxTime = micros();
  while ((cc1101.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1F) != 0x0D) {
    if (micros() - rxTime > 50000) break; // Quit when marcState does not change...
  }
}

//####################################################################
// put CC1101 to send mode
//####################################################################
void enterTx() {
  cc1101.setTxState();
  delay(2);
  long rxTime = micros();
  byte marcState;
  do {
    if (micros() - rxTime > 50000) {
      break; // Quit when marcState does not change...
  }
    marcState = cc1101.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1F;
  } while ((marcState != 0x13) && (marcState != 0x14) && (marcState != 0x15));
}
