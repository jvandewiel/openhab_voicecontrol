OpenHAB Voicecontrol
====================

Openhab configuration with PocketShinx/custom server for voice control and TTS

High level description
----------------------
This is a setup for a voice-to-command system using a Raspberry Pi (RPi) for voice recognition, with a custom "server" that converts voice to commands, which are being send to a different machine running OpenHAB. OpenHAB contains some rules that will execute the command, and send a text response back to the RPi. The RPi then uses text-to-speech (TTS) to provide audio feedback.

The following hardware is used:
- a machine that runs OpenHab (OH Server, I used [one of these] (http://www.minix.com.hk/Products/MINIX-NEOX5.html) but then running Ubuntu/Linux using the information in [this guideline] (http://minixforums.com/threads/linux-on-minix-x5.1388/)
- a machine that runs the "voice to command" server; in this case a Raspberry Pi model B (RPi)
- a machine that runs Festival TTS (in my case this runs on the same machine as OpenHAB)
Additionally, a set of speakers (connected to the RPi) and an audio-in device is required (a Logitech USB Webcam is used here). In this guide there is also a USB WiFi stick. 

### Some notes about performance
The performance and quality of open source speech recognition software under linux is not as good as some commercial alternatives that are available. Combined with the limited processing power of the RPi, this specific use case might not be ready for production, non the least due to the "hacked" server software (no threading, hardcoded stuff, lack of error handling). Currently, speech is recognized in most cases, also due to the limited set of words available in the dictionary. Processing time from speech to command, to action to response is a few seconds - acceptable for me as a proof of concept, but YMMV. 

Installation steps RPi
----------------------
The starting point for the RPi is a clean install with [Moebius] (http://moebiuslinux.sourceforge.net/documentation/installation-guide/), based on [the 1.1.1. image] (http://sourceforge.net/projects/moebiuslinux/files/raspberry.stable/), which is a more bare-bones version of Raspbian, on a 4GB SD card. However, a clean Raspbian can also be used; some of the steps to prep the RPi can then likely be skipped. The following steps are starting with the first boot of the (LAN network connected) RPi and the first SSH session.

### Basic setup
During the first SSH into the RPi (raspberrypi) with user root, password raspi, the config will run. Select autoresize, 2 reboots will follow and SSH again. Run ```apt-get update``` and ```apt-get upgrade``` to get everything up-to-date, and reboot one more time. Change the root password

``` shell
apt-get update
apt-get upgrade
```

Run the config again with , change root password and set the 

- change pwd
- adduser
- set hostname
- install some basic stuff (smb server, nfs-client,
- setup WiFi (details)


* Pocktesphinx
Next steps are the installation of PocketSphinx and the dev environment
 
- continuosly records via audio in, check for speech recognition. If so, display recognize + word
- trigger word -> responds by sending text string to festival_server and playing audio
- if recog, send to openhab -> execute command -> send response (text) -> send to server for audio -> play audio



All machines run a flavour of Ubuntu 12.04/Linux

Setup RPi
---------
For the RPi to be able to use the voice control, harrware is needed that can convert 
speech to a text command. The server software running on the RPi listens continuously 
and will try to process incming audio into text commands. The following steps need to 
be executed on a RPi with a default Rasbian installation

Testing audio in/out
--------------------
Verify the audio in and out are working using the following commands
Execute `arecord something something` and record a couple of seconds of audio. Then execute `aplay something` to verify audio out is also wrking 

Setup PocketSphinx
------------------
Execute the following to download, configure and compile PocketSphinx. More detail can be found at [this URL] (http://www.cmusphinx.edu)
```Shell
mkdir pocketsphinx
wget something
wget something else
unzip
```
- download pocketshpinx stuff (spec dir's)
- download required libs for compiling
 


Compiling PocketSphinx
----------------------
Following librarie sneed to be insalled before 

Compiling step 1
Execute the following commands

Test using `continuous and then some` to verify PocketSphinx is working


----------------------

** original file


pi speech
- festival: echo "Just what do you think you're doing, Dave?" | festival --tts
- espeak: espeak -ven+f3 -k5 -s100 "I've just picked up a fault in the AE35 unit" [-v voice, -k capitals?? -s speed]

mary cmu-slt-hsmm en_US female hmm

http://ubuntuforums.org/showthread.php?t=751169

Installation of Festival
------------------------
- get voices from http://hts.sp.nitech.ac.jp/archives/2.1/
- untar, copy (see ubuntuforums link)
* need to be compiled
wget -c http://downloads.sourceforge.net/project/sp-tk/SPTK/SPTK-3.6/SPTK-3.6.tar.gz?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Fsp-tk%2Ffiles%2FSPTK%2FSPTK-3.6%2F&ts=1377781885&use_mirror=netcologne
wget -c http://hts.sp.nitech.ac.jp/archives/2.2/HTS-2.2_for_HTK-3.4.1.tar.bz2
wget -c http://downloads.sourceforge.net/hts-engine/hts_engine_API-1.07.tar.gz

do configure, make, sudo make install of hts_engine (check hts_engine in cli)
do configure, make, sudo make install of SPTK

also check https://help.ubuntu.com/community/TextToSpeech

// ignore above -> move this to xnas installs script stuff; follow howto above for voices
install festival on xnas, run as server via festival --server, do some security settings in /etc/festival.scm
call from client via  echo "Lets do some testing" | festival_client --server 192.168.1.30 --port 1314 --async --ttw --aucommand 'aplay $FILE'

openhab install on xnas, add openhab user as well who start -> see openhabpi script install; also for other services under openhab user

-- speech recognition
http://stackoverflow.com/questions/17778532/raspberrypi-pocketsphinx-ps3eye-error-failed-to-open-audio-device

on pi: http://codebangers.com/?p=685


sudo apt-get install bison # required
sudo apt-get install libpulse-dev
sudo apt-get install gstreamer0.10-pulseaudio libao4 libasound2-plugins libgconfmm-2.6-1c2 libglademm-2.4-1c2a libpulse-dev libpulse-mainloop-glib0 libpulse-mainloop-glib0-dbg libpulse0 libpulse0-dbg libsox-fmt-pulse paman paprefs pavucontrol pavumeter pulseaudio pulseaudio-dbg pulseaudio-esound-compat pulseaudio-esound-compat-dbg pulseaudio-module-bluetooth pulseaudio-module-gconf pulseaudio-module-jack pulseaudio-module-lirc pulseaudio-module-lirc-dbg pulseaudio-module-x11 pulseaudio-module-zeroconf pulseaudio-module-zeroconf-dbg pulseaudio-utils oss-compat -y

```Shell
wget  http://downloads.sourceforge.net/project/cmusphinx/sphinxbase/0.8/sphinxbase-0.8.tar.gz
tar -xvf sphinxbase-0.8.tar.gz
cd sphinxbase-0.8
./configure
make
sudo make install

wget  wget http://sourceforge.net/projects/cmusphinx/files/pocketsphinx/0.8/pocketsphinx-0.8.tar.gz
tar -xvf pocketsphinx-0.8.tar.gz
cd pocketsphinx-0.8
./configure
make
sudo make install

```

Path settings
```shell
export PYTHONPATH="/usr/local/lib/python2.7/site-packages"
```

Get langauge and acoustic models from
http://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/US%20English%20Generic%20Language%20Model/
http://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/US%20English%20Generic%20Acoustic%20Model/

``` shell
wget -c http://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/US%20English%20Generic%20Language%20Model/en-us.lm.dmp/download 
wget -c http://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/US%20English%20Generic%20Acoustic%20Model/en-us.tar.gz/download
```

run with
- not neccse. after recomp 

` LD_LIBRARY_PATH=/usr/local/lib /usr/local/bin/pocketsphinx_continuous `

Select correct input and more: https://sites.google.com/site/observing/Home/speech-recognition-with-the-raspberry-pi

Remove all pulseaudio, install per http://wiki.roberttwomey.com/Raspberry_Pi#pocketsphinx
Should display in make process that ALSA is being used instead of pulseaudio

Install tool kit per http://kerneldriver.org/blog/2013/02/08/using-pocketsphinx-part-2-using-the-cmu-cambridge-statistical-language-modeling-toolkit/

Source festival https://github.com/zeehio/festival/tree/master/examples

Per http://www.ghatage.com/2012/12/make-pocketsphinx-recognize-new-words/
Create dic/lang model: http://www.speech.cs.cmu.edu/tools/lmtool.html using input file with sentences & words unzip tar file, load with

```shell
pocketsphinx_continuous -adcdev sysdefault -samprate 16000 -dict /home/pi/4203.dic -lm /home/pi/4203.lm (or whatever lm/doc are returned)
```

Pocketsphinx links
------------------
- Features discussion http://sourceforge.net/p/cmusphinx/discussion/help/thread/49e34dff/
- Local lmtools http://scribblej.com/svn/ and http://www.jaivox.com/pocketsphinx.html
- Acoustic model http://cmusphinx.sourceforge.net/wiki/tutorialadapt

Other links
-----------
http://kerneldriver.org/blog/2013/02/08/using-pocketsphinx-part-1-using-the-sphinx-knowledge-base-tool/
http://cmusphinx.sourceforge.net/wiki/sphinxinaction
http://cmusphinx.sourceforge.net/wiki/tutorialpocketsphinx
http://stackoverflow.com/questions/4535208/pocketsphinx-adding-words-and-improving-accuracy
XBMC voice control http://forum.xbmc.org/showthread.php?tid=123621
http://hackaday.com/2013/08/11/voice-controlled-home-automation-uses-raspberry-pi-and-lightwaverf/ 
http://stevenhickson.blogspot.nl/2013/06/installing-and-updating-piauisuite-and.html
http://www.moop.org.uk/index.php/2013/08/10/voice-controlled-lights/

TTS links
---------
TTS http://elinux.org/RPi_Text_to_Speech_%28Speech_Synthesis%29
http://wahelper.brailcom.org/doc/speechd/speech-dispatcher.html and http://devel.freebsoft.org/speechd

Festival source code 
--------------------

https://github.com/zeehio/festival/blob/master/examples/festival_client.c
https://github.com/zeehio/festival/blob/master/examples/festival_client.h
http://www.festvox.org/docs/manual-1.4.3/festival_28.html#SEC129

For compiling, see [here] (http://cmusphinx.sourceforge.net/wiki/tutorialpocketsphinx)
custom continuous.c; compile using
```shell
gcc -o stevo continuous.c -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`
```

Call using
```shell
./stevo -adcdev sysdefault -samprate 16000 -dict /home/pi/4203.dic -lm /home/pi/4203.lm
```
