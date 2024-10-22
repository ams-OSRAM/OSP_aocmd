# excmdint

Example for the command interpreter.


## Introduction

This is an example Python application, using the python library `libosplink` 
to communicate with the OSPlink firmware in the ESP32S3. It only uses the
basic command interpreter, not the commands of OSPlink.

> `libosplink` is still experimental.


## File architecture

This application uses the following support files

- `setup.bat` first file to run, sets up a virtual Python environment.
  You might need to tweak the line that sets the `LOCATION` of the python executable.
- `requirements.txt` is used by `setup.bat` to install packages.
  In this case, the file contains the serial port library and the libosplink.
- `run.bat` actually runs `excmdint.py`.
- `excmdint.py` the script started by `run.bat`.
- `clean.bat` deletes the virtual environment.
- `readme.md` this file.


## Configuration

- Make sure the (ESP on the) OSP32 board is flashed with the 
  [osplink.ino](https://github.com/ams-OSRAM/OSP_aotop/tree/main/examples/osplink) 
  sketch.

- Connect an OSP chain to the OSP32 board (eg SAIDbasic in BiDir).

- Connect the OSP32 board to the PC.

- Open a cmd in `OSP_aocmd\python\excmdint`.

- Run `setup.bat` (check the line `SET LOCATION=C:\programs\python311\`).


## Run

Once the project is configured, run the python app.
We do this via the `run.bat`. 
Make sure Arduino IDE (or any other app to blocks the COM port) is not running.

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

Run `OSP_aocmd\python\cleanall.bat` to remove all generated files.

(end)

