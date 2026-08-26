** This is the firmware which goes with the demo web app
The firmware is designed to advertise and connect to the web bluetooth app in the rest of the repository.
Once connected, it receives commands from the web bluetooth app and plays the corresponding .WAV file.
The audio playback functions allow start/stop/pause, setting volume and displaying current file metadata.
It runs using Zephyr RTOS, and I built the project in Visual Studio Code.
