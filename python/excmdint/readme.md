# excmdint

Example for command interpreter.


## Introduction

This is an example Python application, using the python library `libosplink` 
to communicate with the OSPlink firmware in the ESP32S3. It only uses the
basic command interpreter, not the commands of OSPlink.

> `libosplink` is still experimental.


## File architecture

This application uses the following support files

- `setup.bat` first file to run, sets up a virtual Python environment.
  You might need to tweak the line that sets the `LOCATION` of the python executable.
- `requirements.txt` is used by `setyp.bat` to install packages.
  In this case, the file contains the serial port library and the libosplink.
- `run.bat` actually runs excmdint.
- `excmdint.py` the script started by `run.bat`.
- `clean.bat` deletes the virtual environment.
- `readme.md` this file.


## Run

The OSP32 board must be connected to the PC.
A run should produce the following output.

```
scanning for COM ports...
found ['COM7']
opening COM7

exec('echo line Hello, world!')
-> Hello, world!

exec('echo faults')
-> echo: faults: 0

logging
see __cmdint.log
Done
```

(end)

