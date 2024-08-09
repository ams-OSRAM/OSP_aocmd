# exosplink

Example for OSPlink.


## Introduction

This is an example Python application, using the python library `libosplink` 
to communicate with the OSPlink firmware in the ESP32S3, actually calling commands.

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


## Run

The OSP32 board must be connected to the PC.
A run should produce the following output.

```
Scanning for ports with OSPlink...
  Found ['COM7']
  Opening COM7

resetinit
  loop
  9
clrerror
goactive
setpwmchn(red)
setpwmchn(grn)
setpwmchn(blue)
setpwmchn(red)
setpwmchn(grn)
setpwmchn(blue)
setpwmchn(red)
setpwmchn(grn)
setpwmchn(blue)
setpwmchn(red)
setpwmchn(grn)
...
```

While this is running, the first RGB led connected to the first SAID should
change color (red to green to blue to red etc).

(end)

