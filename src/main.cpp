#include <SoftwareWire.h>
const int sda=12, scl=11;   // lcd communication on pins 11 and 12
SoftwareWire Wire(sda,scl); // rewire lcd controls to some unused pins
                            // See bperrybap's respons in this thread for details:
                            // https://forum.arduino.cc/index.php?topic=448155.0

#include <Arduino.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

#include <Arduino.h>
#include "JC_Button.h"
#include <RelayModule.h>

#define DEBUG
#define INVERT_RELAY false

// Define the relays
RelayModule* _rl_pushArmOut  = new RelayModule(A0, INVERT_RELAY);
RelayModule* _rl_pushArmIn   = new RelayModule(A1, INVERT_RELAY);
RelayModule* _rl_sweepArmOut = new RelayModule(A2, INVERT_RELAY);
RelayModule* _rl_sweepArmIn  = new RelayModule(A3, INVERT_RELAY);
RelayModule* _rl_loadChain   = new RelayModule(A4, INVERT_RELAY);
RelayModule* _rl_unloadChain = new RelayModule(A5, INVERT_RELAY);

// Define the switches
Button _sw_pushArmIn    = Button(2);
Button _sw_pushArmOut   = Button(3);
Button _sw_sweepArmIn   = Button(4);
Button _sw_sweepArmOut  = Button(5);

Button _sw_unloadChain  = Button(6);
Button _sw_baleRowReady = Button(7);
Button _sw_loadIsFull   = Button(8);
Button _sw_rowSwept     = Button(9);

// Defne LCD details
hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

// State machine definitions and string names
enum machineState { HOME, LOAD, SWEEP_ARM_OUT, SWEEP_ARM_IN, PUSH_ARM_OUT, PUSH_ARM_IN, UNLOAD };

const char *_machineStateNames[] = {
  "HOMING", "LOADING", "SWEEP ARM OUT", "SWEEP ARM IN", "PUSH ARM OUT", "PUSH ARM IN", "UNLOADING" };

machineState _machineState;
machineState _previousMachineState;

String _actionMessagePrevious = "";
String _stateMessagePrevious = "";

// Forward declarations
bool doHome();
bool doLoad();
bool doPushArmIn();
bool doPushArmOut();
bool doSweepArmIn();
bool doSweepArmOut();
bool doUnload();
void initLcd();
void initRelays();
void initSwitches();
void lcdClearLine(int i);
void lcdMsg_state(String s);
void lcdMsg_action(String s);
//void printDebugString(String s);
void readSwitches();
void writeSwitchStateChanges();

// Track the state of the unload switch (see explanation in doUnload())
bool _unloadSwitchHasOpened;

// A small delay after the "baleRowReady" switch is tripped. Used for the first
// row only, this allows the row to travel as far as possible before the sweep
// arm is engaged.
int _firstRowLoadDelay;

// Core Arduino setup() function, used here for initilization
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  initSwitches();

  initLcd();

  // Start in the "HOME" state...
  _machineState = HOME;
  _previousMachineState = LOAD;

  _unloadSwitchHasOpened = false;

  _firstRowLoadDelay = 0;
}

// Core Arduino loop() function, used here for FSM control
void loop() {
  // Update all switch states
  readSwitches();

  lcdMsg_state(String(_machineStateNames[_machineState]));

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
          _unloadSwitchHasOpened = false;
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

  _previousMachineState = _machineState;
}

// Home all mechanical functions to start in a known state
bool doHome()
{
  // Turn off unneeded functions
  _rl_loadChain->off();
  _rl_pushArmOut->off();
  _rl_sweepArmOut->off();

  // If the push arm isn't home, start it returning
  if (!_sw_pushArmIn.isPressed())
  {
    lcdMsg_action("PUSH ARM IN");
    _rl_pushArmIn->on();
    return false;
  }
  else
  {
    _rl_pushArmIn->off();
  }

  // If the sweep arm isn't home, start it returning
  if (!_sw_sweepArmIn.isPressed())
  {
    lcdMsg_action("SWEEP ARM IN");
    _rl_sweepArmIn->on();
    return false;
  }
  else
  {
    _rl_sweepArmIn->off();
  }

  // If the unload chain isn't home, start it returning
  // Technicall, we can't control this. Here, we activate the clutch
  // that will allow the chain to move, but the machine must be moved
  // to activate the chain and ulitmately impact the switch state...
  if (!_sw_unloadChain.isPressed())
  {
    lcdMsg_action("UNLOAD CHAIN");
    _rl_unloadChain->on();
    return false;
  }
  else
  {
    _rl_unloadChain->off();
  }

  return true;
}

// Drive the load chain to get more bales
bool doLoad()
{
  // Turn off unneeded functions
  _rl_unloadChain->off();
  _rl_pushArmIn->off();
  _rl_pushArmOut->off();
  _rl_sweepArmIn->off();
  _rl_sweepArmOut->off();

  // If we don't have 2 bales ready to move, just keep loading...
  if (!_sw_baleRowReady.isPressed())
  {
    lcdMsg_action("2-BALES READY");
    _rl_loadChain->on();
    return false;
  }
  else
  {
    // If this is the first row, let it travel for a while longer to better
    // position it for the sweep arm
    if (!_sw_rowSwept.isPressed()) { delay(_firstRowLoadDelay); }
    _rl_loadChain->off();
    return true;
  }
}

// Push the current set of 2-bales across to free up load chamber
bool doPushArmOut()
{
  // Turn off unneeded functions
  _rl_loadChain->off();
  _rl_unloadChain->off();
  _rl_pushArmIn->off();
  _rl_sweepArmIn->off();
  _rl_sweepArmOut->off();

  // If the push arm isn't stroked out and the "full load" switch isn't tripped, activate the push arm
  if (!_sw_pushArmOut.isPressed() && !_sw_loadIsFull.isPressed())
  {
    lcdMsg_action("PUSH ARM OUT or FULL");
    _rl_pushArmOut->on();
    return false;
  }
  else
  {
    _rl_pushArmOut->off();
    return true;
  }
}

// Retract the push arm to load the next set of 2-bales
bool doPushArmIn()
{
  // Turn off unneeded functions
  _rl_loadChain->off();
  _rl_unloadChain->off();
  _rl_pushArmOut->off();
  _rl_sweepArmIn->off();
  _rl_sweepArmOut->off();

  if (!_sw_pushArmIn.isPressed())
  {
    lcdMsg_action("PUSH ARM IN");
    _rl_pushArmIn->on();
    return false;
  }
  else
  {
    _rl_pushArmIn->off();
    return true;
  }
}

// Sweep the first set of 2 bales 90 degrees to the front of the machine
bool doSweepArmOut()
{
  // Turn off unneeded functions
  _rl_loadChain->off();
  _rl_unloadChain->off();
  _rl_pushArmIn->off();
  _rl_pushArmOut->off();
  _rl_sweepArmIn->off();

  // If the sweep arm isn't stroked out, activate the sweep arm
  if (!_sw_sweepArmOut.isPressed())
  {
    lcdMsg_action("SWEEP ARM OUT");
    _rl_sweepArmOut->on();
    return false;
  }
  else
  {
    _rl_sweepArmOut->off();
    return true;
  }
}

// Retract the sweep arm to load the next set of 2-bales
bool doSweepArmIn()
{
  // Turn off unneeded functions
  _rl_loadChain->off();
  _rl_unloadChain->off();
  _rl_pushArmIn->off();
  _rl_pushArmOut->off();
  _rl_sweepArmOut->off();

  if (!_sw_sweepArmIn.isPressed())
  {
    lcdMsg_action("SWEEP ARM IN");
    _rl_sweepArmIn->on();
    return false;
  }
  else
  {
    _rl_sweepArmIn->off();
    return true;
  }
}

// We're full. Unload the 10-bale set.
bool doUnload()
{
  // Turn off unneeded functions
  _rl_loadChain->off();
  _rl_pushArmIn->off();
  _rl_pushArmOut->off();
  _rl_sweepArmIn->off();
  _rl_sweepArmOut->off();

  // Note, the cycle both starts and stops with the same switch being tripped...
  // The switch should be "pressed" at the start of the cycle. Run the unload chain until
  // the switch opens. Then, continue to run the chain until the switch closes again.

  // blindly turn on the unload chain...
  _rl_unloadChain->on();

  // If the unload switch hasn't yet been opened and it's currently pressed, keep waiting...
  if (!_unloadSwitchHasOpened && _sw_unloadChain.isPressed())
  {
    lcdMsg_action("UNLOAD TO OPEN");
    return false;
  }

  // Here, the switch has opened (should be the start of the unload cycle)
  // Remember its state, but keep waiting...
  _unloadSwitchHasOpened = true;

  // If the switch is *NOT* pressed, we're still unloading. Keep waiting...
  if (!_sw_unloadChain.isPressed())
  {
    lcdMsg_action("UNLOAD TO CLOSE");
    return false;
  }

  // Finally, the switch is closed again and we're at the end of the unload state
  _unloadSwitchHasOpened = false;
  _rl_unloadChain->off();

  return true;
}

void initLcd()
{
  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.clear();
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

  #ifdef DEBUG
    //writeSwitchStateChanges();
  #endif
}

void lcdMsg_action(String msg)
{
  if (msg != _actionMessagePrevious)
  {
    _actionMessagePrevious = msg;
    lcdClearLine(2);
    lcd.print("Waiting for switch:");
    lcdClearLine(3);
    lcd.print(msg);

    #ifdef DEBUG
      Serial.println("Waiting for switch: " + msg);
    #endif
  }
}

void lcdMsg_state(String s)
{
  if (s != _stateMessagePrevious)
  {
    _stateMessagePrevious = s;
    String msg = "State: " + s;
    lcdClearLine(0);
    lcd.print(msg);

    #ifdef DEBUG
      Serial.println(msg);
    #endif
  }
}

void lcdClearLine(int line)
{
  lcd.setCursor(0,line);
  for(int n = 0; n < LCD_COLS; n++)
  {
    lcd.print(" ");
  }
  lcd.setCursor(0, line);
}

// void writeSwitchStateChanges()
// {
//   if (_sw_pushArmIn.wasPressed())
//   {
//     Serial.println("    - Push Arm In: Pressed");
//   }

//   if (_sw_pushArmIn.wasReleased())
//   {
//     Serial.println("    - Push Arm In: Released");
//   }

//   if (_sw_pushArmOut.wasPressed())
//   {
//     Serial.println("    - Push Arm Out: Pressed");
//   }

//   if (_sw_pushArmOut.wasReleased())
//   {
//     Serial.println("    - Push Arm Out: Released");
//   }

//   if (_sw_sweepArmIn.wasPressed())
//   {
//     Serial.println("    - Sweep Arm In: Pressed");
//   }

//   if (_sw_sweepArmIn.wasReleased())
//   {
//     Serial.println("    - Sweep Arm In: Released");
//   }

//   if (_sw_sweepArmOut.wasPressed())
//   {
//     Serial.println("    - Sweep Arm Out: Pressed");
//   }

//   if (_sw_sweepArmOut.wasReleased())
//   {
//     Serial.println("    - Sweep Arm Out: Released");
//   }

//   if (_sw_unloadChain.wasPressed())
//   {
//     Serial.println("    - Unload Chain: Pressed");
//   }

//   if (_sw_unloadChain.wasReleased())
//   {
//     Serial.println("    - Unload Chain: Released");
//   }

//   if (_sw_baleRowReady.wasPressed())
//   {
//     Serial.println("    - Bale Row Ready: Pressed");
//   }

//   if (_sw_baleRowReady.wasReleased())
//   {
//     Serial.println("    - Bale Row Ready: Released");
//   }

//   if (_sw_loadIsFull.wasPressed())
//   {
//     Serial.println("    - Load Is Full: Pressed");
//   }

//   if (_sw_loadIsFull.wasReleased())
//   {
//     Serial.println("    - Load Is Full: Released");
//   }

//   if (_sw_rowSwept.wasPressed())
//   {
//     Serial.println("    - Row Swept: Pressed");
//   }

//   if (_sw_rowSwept.wasReleased())
//   {
//     Serial.println("    - Row Swept: Released");
//   }
// }