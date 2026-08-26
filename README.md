## Demo I2S audio web app 
This is a very high level example of an exercise interface, though is essentially a sound board which plays a selection of WAV files through the Xiao nrf52840 board's I2S amplifier (BFF audio breakout + MAX98357 amplifier) depending on which button has been pressed.
It demonstrates the following functionality:
* Bluetooth service which writes to the connected dev board
* A simple audio command structure which is parsed by the dev board
* Start/stop exercise button which plays metronome sound and (optionally) has flashing red VOR x 1 circle

## App firmware
/wav-zephyr-control contains the firmware controlling I2S audio playback and Bluetooth communication.
There is a larger ReadMe in that folder but building the firmware using west build -b xiao_ble allows you to generate your own uf2 file.
Otherwise I have attached a uf2 in this folder as well which hopefully should just work.
