# exosplink

Example for OSPlink.


## Introduction

This is an example Python application, using the python library `libosplink` 
to communicate with the OSPlink firmware in the ESP32S3, actually calling 
commands.

> `libosplink` is still experimental.


## File architecture

This application uses the following support files

- `setup.bat` first file to run, sets up a virtual Python environment.
  You might need to tweak the line that sets the `LOCATION` of the python executable.
- `requirements.txt` is used by `setyp.bat` to install packages.
  In this case, the file contains the serial port library and the libosplink.
- `run.bat` actually runs excmdint.
- `exosplink.py` the script started by `run.bat`.
- `clean.bat` deletes the virtual environment.
- `readme.md` this file.


## Configuration

- Make sure the (ESP on the) OSP32 board is flashed with the 
  [osplink.ino](https://github.com/ams-OSRAM-Group/OSP_aotop/tree/main/examples/osplink) 
  sketch.

- Connect an OSP chain to the OSP32 board (eg SAIDbasic in BiDir).

- Connect the OSP32 board to the PC.

- Open a cmd in `OSP_aocmd\python\exosplink`.

- Run `setup.bat` (check the line `SET LOCATION=C:\programs\python311\`).


## Run

Once the project is configured, run the python app.
We do this via the `run.bat`.
Make sure Arduino IDE (or any other app to blocks the COM port) is not running.

```
Scanning for ports with OSPlink...
  Found ['COM7']
  Opening COM7

resetinit() -> dirmux=bidir last=8
clrerror(0x000)
goactive(0x000)
round 0
 setpwmchn(0x001,red)
 setpwmchn(0x001,grn)
 setpwmchn(0x001,blue)
round 1
 setpwmchn(0x001,red)
 setpwmchn(0x001,grn)
 setpwmchn(0x001,blue)
round 2
 setpwmchn(0x001,red)
 setpwmchn(0x001,grn)
 setpwmchn(0x001,blue)
round 3
 setpwmchn(0x001,red)
 setpwmchn(0x001,grn)
 setpwmchn(0x001,blue)
round 4
 setpwmchn(0x001,red)
 setpwmchn(0x001,grn)
 setpwmchn(0x001,blue)
Done
```

While this is running, the first RGB led connected to the first SAID should
change color (red to green to blue to red etc).

Wait for the 5 rounds to complete (or press ^C to abort).

Run `\OSP_aocmd\python\cleanall.bat` to remove all generated files.

(end)

