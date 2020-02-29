#include <Arduino.h>
#include "JC_Button.h"
#include "Relay.h"

// Define the relays
Relay _rl_pushArmOut(A0, true);
Relay _rl_pushArmIn(A1, true);
Relay _rl_sweepArmOut(A2, true);
Relay _rl_sweepArmIn(A3, true);
Relay _rl_loadChain(A4, true);
Relay _rl_unloadChain(A5, true);

// Define the switches
Button _sw_pushArmIn    = Button(2);
Button _sw_pushArmOut   = Button(3);
Button _sw_sweepArmIn   = Button(4);
Button _sw_sweepArmOut  = Button(5);
Button _sw_unloadChain  = Button(6);
Button _sw_baleRowReady = Button(7);
Button _sw_loadIsFull   = Button(8);
Button _sw_rowSwept     = Button(9);

// State machine definitions
enum machineState { HOME, LOAD, SWEEP_ARM_OUT, SWEEP_ARM_IN, PUSH_ARM_OUT, PUSH_ARM_IN, UNLOAD };
machineState _machineState;

// Forward declarations
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

bool unloadSwitchHasOpened = false;

void setup() {
  Serial.begin(9600);
  initSwitches();
  initRelays();

  // Start up in "HOME" mode...
  _machineState = HOME;
}

void loop() {
  // Update all switch states
  readSwitches();

  // State machine processing
  // Note, this should follow the definition found in the "docs" folder of the project
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
          unloadSwitchHasOpened = false;
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
}

// Home all processes to start in a known state
bool doHome()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
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

// Drive the load chain to get more bales
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

// Push the current set of 2-bales across to free up load chamber
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

// Retract the push arm to load the next set of 2-bales
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

// Sweep the first set of 2 bales 90 degrees to the front of the machine
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

// Retract the sweep arm to load the next set of 2-bales
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

// We're full. Unload the 10-bale set.
bool doUnload()
{
  // Turn off unneeded functions
  _rl_loadChain.turnOff();
  _rl_pushArmIn.turnOff();
  _rl_pushArmOut.turnOff();
  _rl_sweepArmIn.turnOff();
  _rl_sweepArmOut.turnOff();

  // Note, the cycle both starts and stops with the same switch being tripped...
  // The switch should be "pressed" at the start of the cycle. Run the unload chain until
  // the switch opens. Then, continue to run the chain until the switch closes again.

  // blindly turn on the unload chain...
  _rl_unloadChain.turnOn();

  // If the unload switch hasn't yet been opened and it's currently pressed, keep waiting...
  if (!unloadSwitchHasOpened && _sw_unloadChain.isPressed())
  {
    return false;
  }

  // Here, the switch has opened (should be the start of the unload cycle)
  // Remember its state, but keep waiting...
  unloadSwitchHasOpened = true;

  // If the switch is *NOT* pressed, we're still unloading. Keep waiting...
  if (!_sw_unloadChain.isPressed())
  {
    return false;
  }

  // Finally, the switch is closed again and we're at the end of the unload state
  unloadSwitchHasOpened = false;
  _rl_unloadChain.turnOff();

  return true;
}

// Initialize all switches
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

// Initialize all relays
void initRelays()
{
  _rl_pushArmOut.begin();
  _rl_pushArmIn.begin();
  _rl_sweepArmOut.begin();
  _rl_sweepArmIn.begin();
  _rl_loadChain.begin();
  _rl_unloadChain.begin();
}

// Read and update the switch states
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