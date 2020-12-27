#include "Ps2Mouse.h"

static const int PS2_CLOCK = 2;
static const int PS2_DATA  = 17;
static const int RS232_RTS = 3;
static const int RS232_TX  = 4;
static const int LED = 13;

static Ps2Mouse mouse(PS2_CLOCK, PS2_DATA, Ps2Mouse::Mode::remote);

static void sendSerialBit(int data) {
  // Delay between the signals to match 1200 baud
  static const auto usDelay = 1000000 / 1200;
  digitalWrite(RS232_TX, data);
  delayMicroseconds(usDelay);
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
  byte mb = data.middleButton ? 0x20 : 0;
  sendSerialByte(0x40 | lb | rb | ((dy >> 4) & 0xC) | ((dx >> 6) & 0x3));
  sendSerialByte(dx & 0x3F);
  sendSerialByte(dy & 0x3F);
  sendSerialByte(mb);
}

static void initSerialPort() {
  Serial.println("Starting serial port");
  digitalWrite(LED, HIGH);
  digitalWrite(RS232_TX, HIGH);
  delayMicroseconds(10000);
  sendSerialByte('M');
  sendSerialByte('3');
  delayMicroseconds(10000);
  digitalWrite(LED, LOW);

  Serial.println("Listening on RTS");
  void (*resetHack)() = 0;
  attachInterrupt(digitalPinToInterrupt(RS232_RTS), resetHack, FALLING);
}

void setup() {
  // PS/2 Data input must be initialized shortly after power on,
  // or the mouse will not initialize
  pinMode(PS2_DATA, INPUT_PULLUP);
  pinMode(RS232_TX, OUTPUT);
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  initSerialPort();
  Serial.println("Reseting PS/2 mouse");
  mouse.reset();
  Serial.println("Setup done!");
}

void loop() {
  Ps2Mouse::Data data;
  if (mouse.readData(data)) {
    sendToSerial(data);
  }
}
