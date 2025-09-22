# ESP Stepper Server

## Project Goal

- A wireless (wifi and bluetooth, if possible) stepper controller which will be controlled through commands sent over websocket.
- A separate (outside the scope of this project) mobile webapp or remote control device will be made to control it.
- The first project this stepper motor will used in is a lamp with moving arms.
  - Stepper position precision is not that important, but smooth/quiet movement and making sure the project does not destroy itself is important!
  - The stepper in this first project is connected to a threaded rod (like a z-axis in many 3d printers).

## Hardware details

- A wireless stepper controller for LilyGo T-Motor with TMC2209 stepper driver and MT6816 magentic encoder (16384 pulses per rotation)
- The LilyGo T-Motor pcb has a ESP32 pico on it
- There are 3 onboard buttons, but they will only be needed for debugging, not primary control.
- Additionally there will be 2 limit switches, attached to IO21 and IO22, pulled low when hit
- Stepper Motor specs
  - Manufacturer Part Number: 17HS19-2004S1
  - Motor Type: Bipolar Stepper
  - Step Angle: 1.8deg
  - Holding Torque: 59Ncm(83.55oz.in)
  - Rated Current/phase: 2.0A
  - Phase Resistance: 1.4ohms

## Links to information and documentation

- Manufacturer documentation + example code https://github.com/Xinyuan-LilyGO/T-Motor/tree/main

# MoSCoW

## Must haves

- Very close attention paid to the example implementation made by the Manufacturer. This example is contained in main.cpp at the moment (the oled display code has been removed).
- Real time or near real time control of the stepper driver to ensure smooth operation
  - In other project I have used the arkhipenko/TaskScheduler library, but in the example code FreeRTOS is used, seems like a nice moment to learn.
- A modular project setup with separation of concerns between modules
  - The first project will be a lamp with moving arms
  - Code will be re-used in future projects with other form factors.
- Sync of state between webapp and controller. Upon connect the controller should send current values to the webapp, for example:
  - Current step position
  - Current speed and direction, if moving
  - Config, see below
- ElegantOTA (ayushsharma82/ElegantOTA) integration so new firmware can be uploaded wirelessly. (I have example working code, please ask for a pointer to the code if needed)
- Websocket by use of the ESP32Async/ESPAsyncWebServer library. (I have example working code, please ask for a pointer to the code if needed)
- A separate (from control) http server path (with websocket) where debugging/serial output is sent to, in case a client is connected. Since the normal serial port is hard to get to, this is important. 

## Should haves

- Smooth accelleration/decelleration
- Configuration of the TMC2209 stepper driver through Serial connection (see options in main.cpp) (SpreadCycle and StealthChop, anything for smooth operation)
- A way to send config from the webapp to be saved to SPIFF/NVRAM or other filesystem. For example:
  - Accelleration and max speed
- WifiManager (tzapu/WiFiManager) integration to be able to set up and manage wifi config.

## Could have

- 3 physical buttons on the T-Motor pcb should start/stop the stepper.
  - Button 1: send the stepper clockwise until limit switch 1 is hit
  - Button 2: stop all movement immediately
  - Button 3: send the stepper counterclockwise until limit switch 1 is hit
- Custom movement playlist. It would be interesting to have a way to make a playlist or predetermined movement loop. So not fixed positions, but constantly moving.
  - As a stretch goal this sequence could also be sent from the webapp.

## Wont have (yet)

- The board/factory code has support for a small OLED display, but this is not needed/important at this time.
- No need to be able to remotely configure stepper parameters like stepsize/microstepping/max current etc.
