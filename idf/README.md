
### 1. Setup esp framework first
Follow instructions in https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#get-started


### 2. Eclipse all in one setup
https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md


### Useful Commands
. $HOME/esp/esp-idf/export.sh <br>
cd ~/esp/hello_world <br>
idf.py set-target esp32 <br>
idf.py menuconfig <br>
idf.py build <br>
idf.py -p /dev/ttyUSB0 flash <br>
idf.py -p /dev/ttyUSB0 monitor <br>
idf.py -p /dev/ttyUSB0 flash monitor
