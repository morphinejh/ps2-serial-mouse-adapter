#pragma once

#include <Arduino.h>

class Ps2Mouse {
public:

  enum Mode {
    remote,
    stream, 
  };

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
  
  Ps2Mouse(int clockPin, int dataPin, Mode mode);

  bool reset() const;

  bool setScaling(bool flag) const;
  bool setResolution(byte resolution) const;
  bool setSampleRate(byte sampleRate) const;

  bool getSettings(Settings& settings) const;

  bool enableDataReporting() const;
  bool disableDataReporting() const;
  bool readData(Data& data) const;

private:
  struct Impl;

  int m_clockPin;
  int m_dataPin;
  Mode m_mode;
};
