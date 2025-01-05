/*
 * Modifed by Jason Hill
 * 2023/06/18 - Switched digital read/write to direct port manipulation.
 *            - Added constructor for streaming mode
 *            - Changed clock and data pin from 'int' to 'byte'
 *            - Added debug compile time option (Levels: none, 0, 1), see 'ProMicro.h'
 */
#include "ProMicro.h"
#include "Ps2Mouse.h"

namespace {

struct Status {

  byte rightButton   : 1;
  byte middleButton  : 1;
  byte leftButton    : 1;
  byte na2           : 1;
  byte scaling       : 1;
  byte dataReporting : 1;
  byte remoteMode    : 1;
  byte na1           : 1;

  byte resolution;
  byte sampleRate;
};

struct Packet {

  byte leftButton    : 1;
  byte rightButton   : 1;
  byte middleButton  : 1;
  byte na            : 1;
  byte xSign         : 1;
  byte ySign         : 1;
  byte xOverflow     : 1;
  byte yOverflow     : 1;

  byte xMovement;
  byte yMovement;
};

enum class Command {
  disableScaling        = 0xE6,
  enableScaling         = 0xE7,
  setResolution         = 0xE8,
  statusRequest         = 0xE9,
  setStreamMode         = 0xEA,
  readData              = 0xEB,
  resetWrapMode         = 0xEC, // Not implemented
  setWrapMode           = 0xEE, // Not implemented
  reset                 = 0xFF,
  setRemoteMode         = 0xF0,
  getDeviceId           = 0xF2, // Not implemented
  setSampleRate         = 0xF3,
  enableDataReporting   = 0xF4,
  disableDataReporting  = 0xF5,
  setDefaults           = 0xF6, // Not implemented
};

enum class Response {
  isMouse        = 0x00,
  selfTestPassed = 0xAA,
  ack            = 0xFA,
  error          = 0xFC,
  resend         = 0xFE,
};

} // namespace

struct Ps2Mouse::Impl {
  const Ps2Mouse& m_ref;

  void sendBit(int value) const {
    /*
     *while (digitalRead(m_ref.m_clockPin) != LOW) {}
     *digitalWrite(m_ref.m_dataPin, value);
     *while (digitalRead(m_ref.m_clockPin) != HIGH) {}
     */
    while (READCLOCK != 0) {}
      if(value){
        SETDATAHIGH;
      } else {
        SETDATALOW;
      }
    while (READCLOCK == 0) {}//*/
  }

  int recvBit() const {
    /*
     *while (digitalRead(m_ref.m_clockPin) != LOW) {}
     *auto result = digitalRead(m_ref.m_dataPin);
     *while (digitalRead(m_ref.m_clockPin) != HIGH) {}
     *return result;
     */
    while ( READCLOCK != 0) {}
    auto result = READDATA;
    while ( READCLOCK == 0) {}
    return result;
  }

  bool sendByte(byte value) const {

    // Inhibit communication
    SETCLOCKOUT;  //pinMode(m_ref.m_clockPin, OUTPUT);
    SETCLOCKLOW;  //digitalWrite(m_ref.m_clockPin, LOW);
    delayMicroseconds(10);

    // Set start bit and release the clock
    SETDATAOUT;   //pinMode(m_ref.m_dataPin, OUTPUT);
    SETDATALOW;   //digitalWrite(m_ref.m_dataPin, LOW);
    SETCLOCKIN;   //pinMode(m_ref.m_clockPin, INPUT);

    // Send data bits
    byte parity = 1;
    for (auto i = 0; i < 8; i++) {
      byte nextBit = (value >> i) & 0x01;
      parity ^= nextBit;
      sendBit(nextBit);
    }

    // Send parity bit
    sendBit(parity);

    // Send stop bit
    sendBit(1);

    // Enter receive mode and wait for ACK bit
    SETDATAIN;  //pinMode(m_ref.m_dataPin, INPUT);
    return recvBit() == 0;
  }

  bool recvByte(byte& value) const {

    // Enter receive mode
    //pinMode(m_ref.m_clockPin, INPUT);
    //pinMode(m_ref.m_dataPin, INPUT);
      SETCLOCKIN;
      SETDATAIN;

    // Receive start bit
    if (recvBit() != 0) {
      return false;
    }

    // Receive data bits
    value = 0;
    unsigned char parity = 0;
    for (int i = 0; i < 8; i++) {
      byte nextBit = recvBit();
      value |= nextBit << i;
      parity += nextBit;
    }

    // Receive and check parity bit
    parity += recvBit();

    // Receive stop bit
    recvBit();

    if(parity% 2 == 0){
      #if DEBUG>0
        Serial.println("Parity error!!!");
      #endif
      return false;
    } 

    return true;
  }

  template <typename T>
  bool sendData(const T& data) const {
    auto ptr = reinterpret_cast<const byte*>(&data);
    for (auto i = 0; i < sizeof(data); i++) {
      if (!sendByte(ptr[i])) {
        return false;
      }
    }
    return true;
  }

  template <typename T>
  bool recvData(T& data) const {
    auto ptr = reinterpret_cast<byte*>(&data);
    for (auto i = 0u; i < sizeof(data); i++) {
      if (!recvByte(ptr[i])) {
        return false;
      }
    }
    return true;
  }

  bool sendByteWithAck(byte value) const {
    while (true) {
      if (sendByte(value)) {
        byte response;
        if (recvByte(response)) {
#if DEBUG>1
            Serial.print(F("Reponse:"));
            Serial.println(response,HEX);
#endif
          if (response == static_cast<byte>(Response::resend)) {
#if DEBUG>1
            Serial.print(F("Resend Needed:"));
            Serial.println(response,HEX);
#endif
            continue;
          }
          return response == static_cast<byte>(Response::ack);
        }
      }
      return false;
    }
  }

  bool sendCommand(Command command) const {
    return sendByteWithAck(static_cast<byte>(command));
  }

  bool sendCommand(Command command, byte setting) const {
    return sendCommand(command) && sendByteWithAck(setting);
  }

  bool getStatus(Status& status) const {
    return sendCommand(Command::statusRequest) && recvData(status);
  }
};

Ps2Mouse::Ps2Mouse(byte clockPin, byte dataPin)
  : m_clockPin(clockPin), m_dataPin(dataPin), m_stream(false)
{}

Ps2Mouse::Ps2Mouse(byte clockPin, byte dataPin, bool setStream)
  : m_clockPin(clockPin), m_dataPin(dataPin), m_stream(setStream)
{}

bool Ps2Mouse::reset(bool stream) {
  Impl impl{*this};

  if (!impl.sendCommand(Command::reset)) {
      return false;
  }

  byte reply;
  if (!impl.recvByte(reply) || reply != byte(Response::selfTestPassed)) {
      return false;
  }
#if DEBUG>0
  Serial.print(F("1st Reply: "));
  Serial.println(reply,HEX);
#endif
  if (!impl.recvByte(reply) || reply != byte(Response::isMouse)) {
      return false;
  }
#if DEBUG>0
  Serial.print(F("2nd Reply: "));
  Serial.println(reply,HEX);
  Serial.println(F("Attempting to enable reporting."));
#endif
  if (stream){
#if DEBUG>0
    Serial.println(F("Streaming Enabled"));
#endif
    return enableStreaming() && impl.sendCommand(Command::enableDataReporting);    
  }
  else{
#if DEBUG>0
    Serial.println(F("Streaming Disabled"));
#endif
    return disableStreaming() && impl.sendCommand(Command::enableDataReporting);    
  }

}

bool Ps2Mouse::enableStreaming(){
  m_stream=true;
  return Impl{*this}.sendCommand(Command::setStreamMode);
}

bool Ps2Mouse::disableStreaming() {
  m_stream=false;
  return Impl{*this}.sendCommand(Command::setRemoteMode);
}

bool Ps2Mouse::setScaling(bool flag) const {
  return Impl{*this}.sendCommand(flag ? Command::enableScaling : Command::disableScaling);
}

bool Ps2Mouse::setResolution(byte resolution) const {
  return Impl{*this}.sendCommand(Command::setResolution, resolution);
}

bool Ps2Mouse::setSampleRate(byte sampleRate) const {
  return Impl{*this}.sendCommand(Command::setSampleRate, sampleRate);
}

bool Ps2Mouse::getSettings(Settings& settings) const {
  Status status;
  if (Impl{*this}.getStatus(status)) {
    settings.scaling = status.scaling;
    settings.resolution = status.resolution;
    settings.sampleRate = status.sampleRate;
    return true;
  }
  return false;
}

bool Ps2Mouse::readData(Data& data) const {

  Impl impl{*this};

  if (m_stream) {
     //if (digitalRead(m_clockPin) != LOW) {
     if ((PIND &=0b00000100) != 0 ) {
       return false;
     }
  }
  else if (!impl.sendCommand(Command::readData)) {
    return false;
  }

  Packet packet;
  if (!impl.recvData(packet)) {
    return false;
  }

  if(packet.xOverflow || packet.yOverflow){
    #if DEBUG>0
      Serial.println("Movement Overflow!");
    #endif
    return ;
  }

  data.leftButton = packet.leftButton;
  data.middleButton = packet.middleButton;
  data.rightButton = packet.rightButton;
  data.xMovement = (packet.xSign ? -0x100 : 0) | packet.xMovement;
  data.yMovement = (packet.ySign ? -0x100 : 0) | packet.yMovement;
  return true;
}
