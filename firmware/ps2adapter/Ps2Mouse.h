/*
 * Modifed by Jason Hill
 * 2023/06/18 - Added constructor for streaming mode
 *            - Added constructor and reset() for streaming mode
 *            - Changed clock and data pin from 'int' to 'byte'
 */
#pragma once

#include <Arduino.h>

#define maxMovement 254

class Ps2Mouse {
public:
  struct Data {
    bool leftButton;
    bool middleButton;
    bool rightButton;
    int  xMovement;
    int  yMovement;
  };

  struct Settings {
    bool scaling;
    byte resolution;
    byte sampleRate;
  };
  
  Ps2Mouse(byte clockPin, byte dataPin);
  Ps2Mouse(byte clockPin, byte dataPin, bool setStream);

  bool reset(bool stream);

  bool setScaling(bool flag) const;
  bool setResolution(byte resolution) const;
  bool setSampleRate(byte sampleRate) const;
  bool getDeviceID() const; 
  bool getSettings(Settings& settings) const;

  bool enableStreaming();
  bool disableStreaming();
  int readData(Data& data) const;
  void clearData(Data& data) const;
  void accumulate(Data& data,Data& dataT) const;
  

private:
  struct Impl;

  int m_clockPin;
  int m_dataPin;
  bool m_stream;
};
