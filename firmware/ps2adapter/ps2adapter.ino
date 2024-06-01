/*
 * Modifed by Jason Hill
 * 2023/06/18 - Switched digital read/write to direct port manipulation.
 *            - Added constructor for streaming mode
 *            - Changed clock and data pin from 'int' to 'byte'
 *            - Added debug compile time option (Levels: none, 0, 1), see 'ProMicro.h'
 */
#include "ProMicro.h"
#include "Ps2Mouse.h"

static const int RS232_RTS = 3; //PD3 - Port D, bit 3
static const int JP12 = 11;
static const int JP34 = 12;
static const int LED = 13;

Ps2Mouse *mouse;  

// Delay between the signals to match 1200 baud
static const auto usBaudDelay = 1000000 / 1200;
static bool threeButtons = false;


static void sendSerialBit(byte data) {
  if(data)
    RS_SETTXHIGH;
  else
    RS_SETTXLOW;
  delayMicroseconds(usBaudDelay);
}

static void sendSerialByte(byte data) {

  // Start bit
  sendSerialBit(0);

  // Data bits
  for (int i = 0; i < 7; i++) {
    sendSerialBit((data >> i) & 0x01);
  }

  // Stop bit
  sendSerialBit(1);

  // 7+1 bits is normal mouse protocol, but some serial controllers
  // expect 8+1 bits format. We send additional stop bit to stay
  // compatible to that kind of controllers.
  sendSerialBit(1);
}

static void sendToSerial(const Ps2Mouse::Data& data) {
  auto dx = constrain(data.xMovement, -127, 127);
  auto dy = constrain(-data.yMovement, -127, 127);
  byte lb = data.leftButton ? 0x20 : 0;
  byte rb = data.rightButton ? 0x10 : 0;
  sendSerialByte(0x40 | lb | rb | ((dy >> 4) & 0xC) | ((dx >> 6) & 0x3));
  sendSerialByte(dx & 0x3F);
  sendSerialByte(dy & 0x3F);
  if (threeButtons) {
    byte mb = data.middleButton ? 0x20 : 0;
    sendSerialByte(mb);
  }
}

static void initSerialPort() {
#if DEBUG>0
  Serial.println("Starting serial port");
#endif
  RS_SETTXHIGH;
  delayMicroseconds(10000);
  sendSerialByte('M');
  if(threeButtons) {
    sendSerialByte('3');
#if DEBUG>0
    Serial.println("Init 3-buttons mode");
#endif
  }
  delayMicroseconds(10000);
#if DEBUG>0
  Serial.println("Listening on RTS");
#endif
  void (*resetHack)() = 0;
  attachInterrupt(digitalPinToInterrupt(RS232_RTS), resetHack, FALLING);
}

static void initPs2Port() {
#if DEBUG>0
    Serial.println(F("Setting sample rate"));
#endif

  mouse->setSampleRate(40);

  Ps2Mouse::Settings settings;
  if (mouse->getSettings(settings)) {
 #if DEBUG>0
    Serial.print("scaling = ");
    Serial.println(settings.scaling);
    Serial.print("resolution = ");
    Serial.println(settings.resolution);
    Serial.print("samplingRate = ");
    Serial.println(settings.sampleRate);
#endif
  }
}

void setup() {
  // PS/2 Data input must be initialized shortly after power on,
  // or the mouse will not initialize
  PS2_SETDATAIN_UP;
  RS_SETTXOUT;

  pinMode(JP12, INPUT_PULLUP);
  pinMode(JP34, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  threeButtons = digitalRead(JP12);
#if DEBUG>0
  Serial.begin(115200);
#endif
  //Identify streaming mode at initilization
  mouse = new Ps2Mouse(!digitalRead(JP34));
  initSerialPort();
  initPs2Port();
#if DEBUG>0
  Serial.println("Setup done!");
#endif
  digitalWrite(LED, LOW);
}

void loop() {
  Ps2Mouse::Data data;
  if (mouse->readData(data)) {
    sendToSerial(data);
#if DEBUG>1
      Serial.print(data.xMovement);
      Serial.print(",");
      Serial.print(data.yMovement);
      Serial.print(",");
      Serial.print(data.leftButton,HEX);
      Serial.print(",");
      Serial.print(data.middleButton,HEX);
      Serial.print(",");
      Serial.println(data.rightButton,HEX);     
#endif
  }
  //Note: 'mouse' never needs deleted from memory unless testing.
  //delete mouse;
}
