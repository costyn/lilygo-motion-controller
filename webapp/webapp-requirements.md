# WebApp requirements
This requirements file will detail the features that the web app should have.

## Technology & Example project
Since I'm familiar with these technologies, I'd like to use:
- pnmp
- Vite
- React
- Typescript
- shadcn/ui component library
- Tailwind CSS

Please see /Users/costyn/Developer/Projects/LumiApp for inspiration on another control webapp I designed/coauthored with Claude Code.

This project should use the LumiApp project as a good example of how this project should be set up. Modular with separation of concerns and components, etc. 

Please keep all the app project code within the /app directory.

## Desired Features Phase 1
Current features that are there now in the basic index.html are a very nice start. Lets definitely keep them (cards with info). I'm not tied to the layout, feel free to move stuff around.

- Add forward/back buttons that will jog the motor while they are pressed
- Buttons to move to limits of the movement. Where the controller has the limits configured or hits the limit switches.
- Auto-reconnect on disconnect (see LumiApp for code samples)
- Dark/light mode with a ThemeProvider (see LumiApp for code samples)
- A link/button to the ElegantOTA firmware uploader, which is at the path /update/

## Building
- On vite build, it should write the compiled package to the /data directory, ready to be uploaded with pio run --target uploadfs
- Please see the LumiApp vite.config.js, has a closeBundle example where it copies the build to a data directory (in that example to another project)


## Desired Features Phase 2
Once we have all the above working, some nice other features:

- Debug output: because the controller won't normally have a serial connection, it will be useful to have controller debut output on a separate websocket on the path /debug .
  - There should be a button which opens a panel/card. The card will should the serial output stream from the controller and display it. 
- Preset positions. Initially they should be saved to a browser localstorage. In the future when the controller supports this, they could be written to config in nvram there.
- Some sort of playlist editor. 
  - A way to predefine x number of steps 
  - Each step has a position and a speed to get there
  - When saved, the playlist is sent to the controller and saved to config in nvram.