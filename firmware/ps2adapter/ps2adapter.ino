/*
 * Modifed by Jason Hill
 * 2023/06/18 - Switched digital read/write to direct port manipulation.
 *            - Added constructor for streaming mode
 *            - Changed clock and data pin from 'int' to 'byte'
 *            - Added debug compile time option (Levels: none, 0, 1), see 'ProMicro.h'
 */
#include "ProMicro.h"
#include "Ps2Mouse.h"

static const int PS2_CLOCK = 2; //PD2 - Port D, bit 2
static const int PS2_DATA  = 17;//PC3 - Port C, bit 3
static const int RS232_RTS = 3; //PD3 - Port D, bit 3
static const int RS232_TX  = 4; //PD4 - Port D, bit 5
static const int JP12 = 11;
static const int JP34 = 12;
static const int LED = 13;

Ps2Mouse *mouse;
int errorCount = 0;
bool stream = 0;
int pullRate = -1;



Ps2Mouse::Data data;
static bool threeButtons = false;


static void sendToSerial(const Ps2Mouse::Data& data) {
  auto dx = constrain(data.xMovement, -127, 127);
  auto dy = constrain(-data.yMovement, -127, 127);
  byte lb = data.leftButton ? 0x20 : 0;
  byte rb = data.rightButton ? 0x10 : 0;
  byte mb = data.middleButton ? 0x20 : 0;
  byte msg[4];
  msg[0]=(0x40 | lb | rb | ((dy >> 4) & 0xC) | ((dx >> 6) & 0x3));
  msg[1]=(dx & 0x3F);
  msg[2]=(dy & 0x3F);
  msg[3]=mb;
  Serial.write(msg,threeButtons?4:3);  
}

static void initSerialPort() {
  #if DEBUG<=0
    Serial.begin(1200,SERIAL_7N1);
     byte msg[2];
      msg[0]='M';
      msg[1]='3';
      Serial.write(msg,threeButtons?2:1);
  #else
  //In debug mode you dont get mouse movements writen to serial
    Serial.begin(115200);
    Serial.println("Starting serial port");
  #endif
  void (*resetHack)() = 0;
  attachInterrupt(digitalPinToInterrupt(RS232_RTS), resetHack, FALLING);
}

static void initPs2Port() {
  errorCount = 0;
  stream = !digitalRead(JP34);
#if DEBUG>0
  Serial.println("Reseting PS/2 mouse");
#endif
  //TODO: Cleanup
  //Identify streaming mode or not.
  mouse->reset(stream);
  mouse->setResolution(2);
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
    pullRate = settings.sampleRate;
  }
    mouse->clearData(data);
}

void setup() {
  // PS/2 Data input must be initialized shortly after power on,
  // or the mouse will not initialize
  pinMode(PS2_DATA, INPUT_PULLUP);
  pinMode(RS232_TX, OUTPUT);
  pinMode(JP12, INPUT_PULLUP);
  pinMode(JP34, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  threeButtons = digitalRead(JP12);
  digitalWrite(LED, HIGH);

  initSerialPort();
  mouse = new Ps2Mouse(PS2_CLOCK, PS2_DATA, !digitalRead(JP34));
  initPs2Port();
#if DEBUG>0
  Serial.println("Setup done!");
#endif
  digitalWrite(LED, LOW);
  
}



void loop() {
  Ps2Mouse::Data newData;
   if(!stream)
        delayMicroseconds(1000000/pullRate);
  int status = mouse->readData(newData);
  if (status==0) {
    mouse->accumulate(newData,data);
    if (Serial.availableForWrite() >= SERIAL_TX_BUFFER_SIZE-1){
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
      mouse->clearData(data);
    }
  }else if(status>1){
    errorCount++;
    #if DEBUG>0
     Serial.println("Packet error");
    #endif
    if(errorCount>RESETON){
      initPs2Port();
    }
  }

  //Note: 'mouse' never needs deleted from memory unless testing.
  //delete mouse;
}
