#include <Arduino.h>
#include "JC_Button.h"
#include "Relay.h"

void initSwitches();
void readSwitches();
void initRelays();

bool doHome();
bool doLoad();
bool doPushArmOut();
bool doPushArmIn();
bool doSweepArmOut();
bool doSweepArmIn();
bool doUnload();

Relay _rl_pushArmOut(A0, true);
Relay _rl_pushArmIn(A1, false);
Relay _rl_sweepArmOut(A2, true);
Relay _rl_sweepArmIn(A3, true);
Relay _rl_loadChain(A4, true);
Relay _rl_unloadChain(A5, true);

Button _sw_pushArmIn    = Button(2);
Button _sw_pushArmOut   = Button(3);
Button _sw_sweepArmIn   = Button(4);
Button _sw_sweepArmOut  = Button(5);
Button _sw_unloadChain  = Button(6);
Button _sw_baleRowReady = Button(7);
Button _sw_loadIsFull   = Button(8);
Button _sw_rowSwept     = Button(9);

enum machineState { HOME, LOAD, SWEEP_ARM_OUT, SWEEP_ARM_IN, PUSH_ARM_OUT, PUSH_ARM_IN, UNLOAD };
machineState _machineState;

void setup() {
  Serial.begin(9600);
  initSwitches();
  initRelays();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  readSwitches();

  switch (_machineState) {
    case HOME:
      if (doHome())
      {
        _machineState = LOAD;
      }
      break;
    case LOAD:
      if (doLoad())
      {
        if (_sw_rowSwept.isPressed())
        {
          _machineState = PUSH_ARM_OUT;
        }
        else
        {
          _machineState = SWEEP_ARM_OUT;
        }
      }
      break;
    case SWEEP_ARM_OUT:
      if (doSweepArmOut())
      {
        _machineState = SWEEP_ARM_IN;
      }
      break;
    case SWEEP_ARM_IN:
      if (doSweepArmIn())
      {
        _machineState = LOAD;
      }
      break;
    case PUSH_ARM_OUT:
      if (doPushArmOut())
      {
        _machineState = PUSH_ARM_IN;
      }
      break;
    case PUSH_ARM_IN:
      if (doPushArmIn())
      {
        if (_sw_loadIsFull.isPressed())
        {
          _machineState = UNLOAD;
        }
        else
        {
          _machineState = LOAD;
        }

      }
      break;
    case UNLOAD:
      if (doUnload())
      {
        _machineState = LOAD;
      }
    break;
  }
  // static bool ledState;
  // if (_sw_baleRowReady.wasPressed())
  // {
  //   ledState = !ledState;
  //   digitalWrite(LED_BUILTIN, ledState);
  //   !ledState ? _rl_unloadChain.turnOn() : _rl_unloadChain.turnOff();
  // }
}

bool doHome()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
  _rl_unloadChain.turnOff();
  _rl_pushArmOut.turnOff();
  _rl_sweepArmOut.turnOff();

  // If the push arm isn't home, start it returning
  if (!_sw_pushArmIn.isPressed())
  {
    _rl_pushArmIn.turnOn();
    return false;
  }
  else
  {
    _rl_pushArmIn.turnOff();
  }


  // If the sweep arm isn't home, start it returning
  if (!_sw_sweepArmIn.isPressed())
  {
    _rl_sweepArmIn.turnOn();
    return false;
  }
  else
  {
    _rl_sweepArmIn.turnOff();
  }

  // If the unload chain isn't home, start it returning (?????????????)
  if (!_sw_unloadChain.isPressed())
  {
    _rl_unloadChain.turnOn();
    return false;
  }
  else
  {
    _rl_unloadChain.turnOff();
  }

  return true;
}

bool doLoad()
{
  // Turn off unneeded functions
  _rl_unloadChain.turnOff();
  _rl_pushArmIn.turnOff();
  _rl_pushArmOut.turnOff();
  _rl_sweepArmIn.turnOff();
  _rl_sweepArmOut.turnOff();

  // If we don't have 2 bales to push, just keep loading...
  if (!_sw_baleRowReady.isPressed())
  {
    _rl_loadChain.turnOn();
    return false;
  }
  else
  {
    _rl_loadChain.turnOff();
    return true;
  }
}

bool doPushArmOut()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
  _rl_unloadChain.turnOff();
  _rl_pushArmIn.turnOff();
  _rl_sweepArmIn.turnOff();
  _rl_sweepArmOut.turnOff();

  // If the push arm isn't stroked out and the "full load" switch isn't tripped, activate the push arm
  if (!_sw_pushArmOut.isPressed() && !_sw_loadIsFull.isPressed())
  {
    _rl_pushArmOut.turnOn();
    return false;
  }
  else
  {
    _rl_pushArmOut.turnOff();
    return true;
  }
}

bool doPushArmIn()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
  _rl_unloadChain.turnOff();
  _rl_pushArmOut.turnOff();
  _rl_sweepArmIn.turnOff();
  _rl_sweepArmOut.turnOff();

  if (!_sw_pushArmIn.isPressed())
  {
    _rl_pushArmIn.turnOn();
    return false;
  }
  else
  {
    _rl_pushArmIn.turnOff();
    return true;
  }
}

bool doSweepArmOut()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
  _rl_unloadChain.turnOff();
  _rl_pushArmIn.turnOff();
  _rl_pushArmOut.turnOff();
  _rl_sweepArmIn.turnOff();

  // If the sweep arm isn't stroked out, activate the sweep arm
  if (!_sw_sweepArmOut.isPressed())
  {
    _rl_sweepArmOut.turnOn();
    return false;
  }
  else
  {
    _rl_sweepArmOut.turnOff();
    return true;
  }
}

bool doSweepArmIn()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
  _rl_unloadChain.turnOff();
  _rl_pushArmIn.turnOff();
  _rl_pushArmOut.turnOff();
  _rl_sweepArmOut.turnOff();

  if (!_sw_sweepArmIn.isPressed())
  {
    _rl_sweepArmIn.turnOn();
    return false;
  }
  else
  {
    _rl_sweepArmIn.turnOff();
    return true;
  }
}

bool doUnload()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
  _rl_pushArmIn.turnOff();
  _rl_pushArmOut.turnOff();
  _rl_sweepArmIn.turnOff();
  _rl_sweepArmOut.turnOff();

  if (!_sw_unloadChain.isPressed())
  {
    _rl_unloadChain.turnOn();
    return false;
  }
  else
  {
    _rl_unloadChain.turnOff();
    return true;
  }
}

void initSwitches()
{
  _sw_pushArmIn.begin();
  _sw_pushArmOut.begin();
  _sw_sweepArmIn.begin();
  _sw_sweepArmOut.begin();
  _sw_unloadChain.begin();
  _sw_baleRowReady.begin();
  _sw_loadIsFull.begin();
  _sw_rowSwept.begin();
}

void initRelays()
{
  _rl_pushArmOut.begin();
  _rl_pushArmIn.begin();
  _rl_sweepArmOut.begin();
  _rl_sweepArmIn.begin();
  _rl_loadChain.begin();
  _rl_unloadChain.begin();
}

void readSwitches()
{
  _sw_pushArmIn.read();
  _sw_pushArmOut.read();
  _sw_sweepArmIn.read();
  _sw_sweepArmOut.read();
  _sw_unloadChain.read();
  _sw_baleRowReady.read();
  _sw_loadIsFull.read();
  _sw_rowSwept.read();
}