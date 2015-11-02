Make Nintendo Wii extension controller talk to other devices which support standard USB HID game controllers.  Uses Atmel AVR atmega168 microcontroller.

**Features:**
  * Emulates USB HID Game controller, no drivers in Windows
  * Auto detects nunchuck or classic controller (CC) on power-up
  * Button mapping for classic controller compatible with PS3

**Limitations:**
  * PS home button doesn’t work on PS3
  * Joysticks are not calibrated
  * Only left shoulder pressure axis on classic controller is sent to USB
  * Nunchuck accelerometer data reduced to 8-bit when sent to host

**Directions:**
  1. Plug Wii extension controller into device
  1. Plug USB cable to device
  1. Computer/PS3 should recognize the device as a gamepad

**Room for improvement**
  1. If possible, have optimized HID report descriptor for nunchuck and CC. This would probably allow for all bits of accelerometer data to be passed to USB as well as right shoulder on CC.
  1. Divide the report into segments, allowing more than 8 bytes of data to be sent.
  1. Allow other devices (guitar, drum controllers, etc)

**Some Media**

---

![http://home.manhattan.edu/~ananda.das/hookup.png](http://home.manhattan.edu/~ananda.das/hookup.png)
![http://farm4.static.flickr.com/3503/3302221986_bb186528fd.jpg](http://farm4.static.flickr.com/3503/3302221986_bb186528fd.jpg)

---

**Thanks**
I posted credits in the documentation but I think i should put it here as well.
AVR-USB: http://www.obdev.at/products/avrusb/index.html
AVR-LIB for serial and printing library: http://www.mil.ufl.edu/~chrisarnold/components/microcontrollerBoard/AVR/avrlib/
Win-AVR http://winavr.sourceforge.net/
Thanks to friend for letting me borrow PS3 arcade stick to snoop USB data.
Thanks you Arduino for making physical computing more mainstream and its Twi library.
Also the playground wiki and its contributors
http://www.arduino.cc/playground/Main/WiiChuckClass
http://www.arduino.cc/playground/Main/WiiClassicController