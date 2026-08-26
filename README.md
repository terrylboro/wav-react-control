## Demo I2S audio web app 
This is a very high level example of an exercise interface, though is essentially a sound board which plays a selection of WAV files through the nrf52840 board's I2S amplifier depending on which button has been pressed.
It demonstrates the following functionality:
* Bluetooth service which writes to the connected dev board
* A simple audio command structure which is parsed by the dev board
* Start/stop exercise button which plays metronome sound and (optionally) has flashing red VOR x 1 circle
