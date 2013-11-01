OpenHAB Voicecontrol
====================

Openhab configuration with PocketShinx/custom server for voice control and TTS

Following steps need to be executed; assumption on sserver is as follows:
- 1 machine that runs OpenHab (OH Server)
- 1 machine that runs the "voice to command" server; I used a Raspberry Pi (RPi)
- 1 machine that runs Festival TTS (in my case this runs on the same machine as OpenHAB)

All machines run s a flavour of Ubuntu 12.04/Linux

Setup RPi
=========
For the RPi to be able to use the voice control, harrware is needed that can convert 
speech to a text command. The server software running on the RPi listens continuously 
and will try to process incming audio into text commands. The following steps need to 
be execute don a RPi with a default Rasbian installation

- download pocketshpinx stuff (spec dir's)
- download required libs for compiling
 

