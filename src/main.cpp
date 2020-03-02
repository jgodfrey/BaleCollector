#include <Arduino.h>
#include "JC_Button.h"
#include "Relay.h"

bool DEBUG = true;

// Define the relays
// arg1: pin #, arg2, Normally open
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
const char *_machineStateNames[] = {
  "Home", "Load", "Sweep Arm Out", "Sweep Arm In", "Push Arm Out", "Push Arm In", "Unload" };

machineState _machineState;
machineState _previousMachineState;

// Forward declarations
void initSwitches();
void readSwitches();
void writeSwitchStateChanges();
void initRelays();
bool doHome();
bool doLoad();
bool doPushArmOut();
bool doPushArmIn();
bool doSweepArmOut();
bool doSweepArmIn();
bool doUnload();

// Track the state of the unload switch (see explanaiton in doUnload())
bool unloadSwitchHasOpened = false;

// Core Arduino setup() function, used here for initilization
void setup() {
  if (DEBUG)
  {
    Serial.begin(115200);
  }
  initSwitches();
  initRelays();

  // Start in the "HOME" state...
  _machineState = HOME;
  _previousMachineState = LOAD;
}

// Core Arduino loop() function, used here for FSM control
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

    if (DEBUG && _machineState != _previousMachineState)
    {
      Serial.println("Changing to state: " + String(_machineStateNames[_machineState]));
    }

    _previousMachineState = _machineState;
}

// Home all mechanical functions to start in a known state
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

// Read and update all switch states
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

  if (DEBUG)
  {
    writeSwitchStateChanges();
  }
}

void writeSwitchStateChanges()
{
  if (_sw_pushArmIn.wasPressed())
  {
    Serial.println("Push Arm In: Pressed");
  }

  if (_sw_pushArmIn.wasReleased())
  {
    Serial.println("Push Arm In: Released");
  }

  if (_sw_pushArmOut.wasPressed())
  {
    Serial.println("Push Arm Out: Pressed");
  }

  if (_sw_pushArmOut.wasReleased())
  {
    Serial.println("Push Arm Out: Released");
  }

  if (_sw_sweepArmIn.wasPressed())
  {
    Serial.println("Sweep Arm In: Pressed");
  }

  if (_sw_sweepArmIn.wasReleased())
  {
    Serial.println("Sweep Arm In: Released");
  }

  if (_sw_sweepArmOut.wasPressed())
  {
    Serial.println("Sweep Arm Out: Pressed");
  }

  if (_sw_sweepArmOut.wasReleased())
  {
    Serial.println("Sweep Arm Out: Released");
  }

  if (_sw_unloadChain.wasPressed())
  {
    Serial.println("Unload Chain: Pressed");
  }

  if (_sw_unloadChain.wasReleased())
  {
    Serial.println("Unload Chain: Released");
  }

  if (_sw_baleRowReady.wasPressed())
  {
    Serial.println("Bale Row Ready: Pressed");
  }

  if (_sw_baleRowReady.wasReleased())
  {
    Serial.println("Bale Row Ready: Released");
  }

  if (_sw_loadIsFull.wasPressed())
  {
    Serial.println("Load Is Full: Pressed");
  }

  if (_sw_loadIsFull.wasReleased())
  {
    Serial.println("Load Is Full: Released");
  }

  if (_sw_rowSwept.wasPressed())
  {
    Serial.println("Row Swept: Pressed");
  }

  if (_sw_rowSwept.wasReleased())
  {
    Serial.println("Row Swept: Released");
  }
}