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
During the first SSH into the RPi (raspberrypi) with user root, password raspi, the config will run. Select autoresize, 2 reboots will follow and SSH again. Run ```apt-get update``` and ```apt-get upgrade``` to get everything up-to-date, and reboot one more time. Change the root password with ```passwd``` and add a new user, which will be running the server, with ```useradd pi``` and ```passwd pi``` (need to enable SSH for this one, disable for root), create the pi home folder with ```mkdir /home/pi``` and ```chown pi:pi /home/pi``` . Then ```apt-get install samba nfs-common``` to setup NFS (client) and Samba (server), run ```nano /etc/samba/smb.conf``` and add the ```/home/pi``` folder to the samba config (so it can be accessed from a Windows machine) by adding the following tot the file:

```shell
[pihome]
force user = pi
force group = pi
comment = Home pi
writeable = yes
public = yes
path = /home/pi/
```        

Run ```service samba restart``` and check if the RPi is accessible.

### Alsa setup
PocketSphinx can both be compiled for Alsa and pulseaudio; this setup uses Alsa (which comes with the Moebius distro) but some configuration is required. Detailed info can be found [here] (http://www.techrepublic.com/article/configuring-linux-sound-services-with-alsa/); this just runs through the commands to configure the USB webcam as default recording device.  

First, test the audio recording and playback using the following commands; this will record whatever is said via the USB webcam and play it back through the connected loudspeaker (the recording can be stopped with ```ctrl-z``` (assuming the USB webcam is ```plughw:1,0```) and should at least confirm the hardware is working.

```
arecord -D plughw:1,0 test.wav
aplay test.wav
```

The following can be skipped if voiceserver is called with the correct hw id; If you cannot hear what was just recorded, there might be a problem with the USB webcam or the speakers; DuckDuckGo or Google is your friend. What is key is to find the correct recording device, in this case that is ```plughw:1,0```.

~~Next step is to configure Alsa, so it will use the USB webcam by default; running ```arecord -D sysdefault test2.wav``` probably gives an ```audio open error: No such file or directory``` which needs to be fixed. Execute a ````nano ``` and modify the line ```options cx88_alsa index=0``` to ```options cx88_alsa index=0``` (this is based on the assumption that the device cx88 is the USB webcam). Then reload the config using ``` alsa force-reload```. Run another test by running: ```arecord -D sysdefault test2.wav``` and ```aplay test2.wav```~~

### PocketSphinx setup
Go to the new user's home folder with ```cd /home/pi/``` and run the following to get the required PocketSphinx libraries and the required development files (including the Alsa headers, see [here] (https://sites.google.com/site/observing/Home/speech-recognition-with-the-raspberry-pi))

```Shell
apt-get install gcc make bison libasound2-dev

wget http://downloads.sourceforge.net/project/cmusphinx/sphinxbase/0.8/sphinxbase-0.8.tar.gz
tar -xvf sphinxbase-0.8.tar.gz
cd sphinxbase-0.8
./configure
make
make install

wget http://sourceforge.net/projects/cmusphinx/files/pocketsphinx/0.8/pocketsphinx-0.8.tar.gz
tar -xvf pocketsphinx-0.8.tar.gz
cd pocketsphinx-0.8
./configure
make
make install

mkdir voiceserver
cd voiceserver
wget https://raw.github.com/jvandewiel/openhab_voicecontrol/master/voiceserver.c
cp ../pocketsphinx-0.8/include/pocketsphinx.h .

gcc -o voiceserver voiceserver.c -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`
```

The next step is to test if voice-recognition is actually working. This will use the default dictionary/language model, which is too large and thus slow, so we need to fix that. But first a test: in the voiceserver folder, run 
```./voiceserver -adcdev plughw:1,0 -samprate 16000``` and try to speak a few words when the prompt states ```READY```, to see if they are recognized. The program can be exited using ```ctrl-z```. Saying "steven" is a trigger, should be recognized and you should see the following in the console:

```
[other output deleted]
INFO: voiceserver.c(549): ./voiceserver COMPILED ON: Nov 20 2013, AT: 15:16:24

Warning: Could not find Capture element
READY....
Listening...
Recording is stopped, start recording with ad_start_rec
Stopped listening, please wait...
INFO: cmn_prior.c(121): cmn_prior_update: from < 56.00 -3.00  1.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00 >
INFO: cmn_prior.c(139): cmn_prior_update: to   < 44.34 -0.24  2.09  0.66 -0.48 -0.50  0.34 -0.36  0.22 -0.63 -0.24  0.31  0.38 >
INFO: ngram_search_fwdtree.c(1549):     1295 words recognized (16/fr)
INFO: ngram_search_fwdtree.c(1551):   249713 senones evaluated (3045/fr)
INFO: ngram_search_fwdtree.c(1553):   332901 channels searched (4059/fr), 35646 1st, 40978 last
INFO: ngram_search_fwdtree.c(1557):     3185 words for which last channels evaluated (38/fr)
INFO: ngram_search_fwdtree.c(1560):    28344 candidate words for entering last phone (345/fr)
INFO: ngram_search_fwdtree.c(1562): fwdtree 1.79 CPU 2.183 xRT
INFO: ngram_search_fwdtree.c(1565): fwdtree 1.81 wall 2.207 xRT
INFO: ngram_search_fwdflat.c(302): Utterance vocabulary contains 48 words
INFO: ngram_search_fwdflat.c(937):      567 words recognized (7/fr)
INFO: ngram_search_fwdflat.c(939):    56419 senones evaluated (688/fr)
INFO: ngram_search_fwdflat.c(941):    64144 channels searched (782/fr)
INFO: ngram_search_fwdflat.c(943):     2865 words searched (34/fr)
INFO: ngram_search_fwdflat.c(945):     2279 word transitions (27/fr)
INFO: ngram_search_fwdflat.c(948): fwdflat 0.23 CPU 0.280 xRT
INFO: ngram_search_fwdflat.c(951): fwdflat 0.23 wall 0.284 xRT
INFO: ngram_search.c(1266): lattice start node <s>.0 end node </s>.73
INFO: ngram_search.c(1294): Eliminated 0 nodes before end node
INFO: ngram_search.c(1399): Lattice has 81 nodes, 188 links
INFO: ps_lattice.c(1365): Normalizer P(O) = alpha(</s>:73:80) = -526400
INFO: ps_lattice.c(1403): Joint P(O,S) = -533720 P(S|O) = -7320
INFO: ngram_search.c(888): bestpath 0.01 CPU 0.012 xRT
INFO: ngram_search.c(891): bestpath 0.02 wall 0.021 xRT
000000000: steven
Processing hyp...
READY....

```

### Dictionary setup
The default installation of PocketSphinx comes with a large dictionary of possible words. This is slow, and also too much - if we only want to use some limited commands we can speed things up significantly. In addition, there is a smaller change of 'misunderstanding'. Additional details on a DIY procedure can be found [here] (http://kerneldriver.org/blog/2013/02/08/using-pocketsphinx-part-2-using-the-cmu-cambridge-statistical-language-modeling-toolkit/); the easy way is to use [this] (http://www.speech.cs.cmu.edu/tools/lmtool-new.html). Create a new text file (e.g. custom_dic.txt) and fill it with the sentences you want to be recognized. An example could be:

```
hello steven
lights on
lights off
lights down
lights up
set alarm to 
one
two 
three
four
five
six
seven
eight
nine
ten
eleven
twelve
play music
play movie
snooze alarm
stop alarm
```

Upload the text file using the BROWSE button on [this] (http://www.speech.cs.cmu.edu/tools/lmtool-new.html) page and click on COMPILE KNOWLEDGE BASE. This will take you to the next page; copy the link to the tar file (somewhere on top) and wget it (in this case, it was http://www.speech.cs.cmu.edu/tools/product/1384963831_29749/TAR2693.tgz - note that the file will be deleted about 30 minutes after it is generated). 

Untar the file in ```/home/pi``` using ```tar xvf TAR2693.tgz``` (or whatever the filename was). Now it should be possible to run the voice recognition with the custom dictionary (note the filenames here - it depends on what the tar file was named!) using ```./voiceserver -adcdev plughw:1,0 -samprate 16000 -dict /home/pi/2693.dic -lm /home/pi/2693.lm``` and check if commands are recognized. An example output is shown below

```
[other output deleted]

Warning: Could not find Capture element
READY....
Listening...
Recording is stopped, start recording with ad_start_rec
Stopped listening, please wait...
INFO: cmn_prior.c(121): cmn_prior_update: from < 56.00 -3.00  1.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00 >
INFO: cmn_prior.c(139): cmn_prior_update: to   < 45.55  1.03  0.19 -0.90  0.07  0.38  0.46  0.42  0.63 -0.66 -0.95 -0.14 -0.00 >
INFO: ngram_search_fwdtree.c(1549):      837 words recognized (11/fr)
INFO: ngram_search_fwdtree.c(1551):    27110 senones evaluated (361/fr)
INFO: ngram_search_fwdtree.c(1553):    16759 channels searched (223/fr), 2130 1st, 12153 last
INFO: ngram_search_fwdtree.c(1557):     1234 words for which last channels evaluated (16/fr)
INFO: ngram_search_fwdtree.c(1560):     1036 candidate words for entering last phone (13/fr)
INFO: ngram_search_fwdtree.c(1562): fwdtree 0.16 CPU 0.213 xRT
INFO: ngram_search_fwdtree.c(1565): fwdtree 1.64 wall 2.184 xRT
INFO: ngram_search_fwdflat.c(302): Utterance vocabulary contains 19 words
INFO: ngram_search_fwdflat.c(937):      225 words recognized (3/fr)
INFO: ngram_search_fwdflat.c(939):    33084 senones evaluated (441/fr)
INFO: ngram_search_fwdflat.c(941):    26742 channels searched (356/fr)
INFO: ngram_search_fwdflat.c(943):     1287 words searched (17/fr)
INFO: ngram_search_fwdflat.c(945):      959 word transitions (12/fr)
INFO: ngram_search_fwdflat.c(948): fwdflat 0.10 CPU 0.133 xRT
INFO: ngram_search_fwdflat.c(951): fwdflat 0.10 wall 0.136 xRT
INFO: ngram_search.c(1214): </s> not found in last frame, using ON.73 instead
INFO: ngram_search.c(1266): lattice start node <s>.0 end node ON(2).51
INFO: ngram_search.c(1294): Eliminated 15 nodes before end node
INFO: ngram_search.c(1399): Lattice has 51 nodes, 34 links
INFO: ps_lattice.c(1365): Normalizer P(O) = alpha(ON(2):51:73) = -500840
INFO: ps_lattice.c(1403): Joint P(O,S) = -501010 P(S|O) = -170
INFO: ngram_search.c(888): bestpath 0.00 CPU 0.000 xRT
INFO: ngram_search.c(891): bestpath 0.01 wall 0.009 xRT
000000000: LIGHTS ON
Processing hyp...
READY....
Listening...
Recording is stopped, start recording with ad_start_rec
Stopped listening, please wait...
INFO: cmn_prior.c(121): cmn_prior_update: from < 45.55  1.03  0.19 -0.90  0.07  0.38  0.46  0.42  0.63 -0.66 -0.95 -0.14 -0.00 >
INFO: cmn_prior.c(139): cmn_prior_update: to   < 44.27  0.08  0.03 -1.10 -0.00  0.29  0.27  0.53  0.69 -0.97 -0.63 -0.17  0.11 >
INFO: ngram_search_fwdtree.c(1549):      960 words recognized (11/fr)
INFO: ngram_search_fwdtree.c(1551):    31580 senones evaluated (351/fr)
INFO: ngram_search_fwdtree.c(1553):    19936 channels searched (221/fr), 2580 1st, 14436 last
INFO: ngram_search_fwdtree.c(1557):     1506 words for which last channels evaluated (16/fr)
INFO: ngram_search_fwdtree.c(1560):     1110 candidate words for entering last phone (12/fr)
INFO: ngram_search_fwdtree.c(1562): fwdtree 0.20 CPU 0.222 xRT
INFO: ngram_search_fwdtree.c(1565): fwdtree 1.79 wall 1.986 xRT
INFO: ngram_search_fwdflat.c(302): Utterance vocabulary contains 22 words
INFO: ngram_search_fwdflat.c(937):      291 words recognized (3/fr)
INFO: ngram_search_fwdflat.c(939):    37576 senones evaluated (418/fr)
INFO: ngram_search_fwdflat.c(941):    30242 channels searched (336/fr)
INFO: ngram_search_fwdflat.c(943):     1555 words searched (17/fr)
INFO: ngram_search_fwdflat.c(945):     1166 word transitions (12/fr)
INFO: ngram_search_fwdflat.c(948): fwdflat 0.12 CPU 0.133 xRT
INFO: ngram_search_fwdflat.c(951): fwdflat 0.12 wall 0.137 xRT
INFO: ngram_search.c(1266): lattice start node <s>.0 end node </s>.73
INFO: ngram_search.c(1294): Eliminated 0 nodes before end node
INFO: ngram_search.c(1399): Lattice has 54 nodes, 65 links
INFO: ps_lattice.c(1365): Normalizer P(O) = alpha(</s>:73:88) = -562732
INFO: ps_lattice.c(1403): Joint P(O,S) = -564769 P(S|O) = -2037
INFO: ngram_search.c(888): bestpath 0.01 CPU 0.011 xRT
INFO: ngram_search.c(891): bestpath 0.01 wall 0.008 xRT
000000001: LIGHTS OFF
Processing hyp...
READY....
```

Using the 'steven'keyword will result in some error messagesm, since festival is not up and running, which is the next step. 

### Festival setup and configuration
For performance reasons, this should be done on a different machine, but too keep thing easier, the RPi is used. Execute  ```apt-get festival``` and check if this works by calling ```echo "Give me some audiofeedback" | festival --tts```. Additonal (better) voices etc., can be installed using [this guide] (http://ubuntuforums.org/showthread.php?t=751169) - it is definitely worth the extra effort! To make festival run as a server, execute ```festival --server &``` - default it runs on port 1314, and the voiceserver is hardcoded to localhost (127.0.0.1) and this port (this really should be some parameters instead). Re-run voiceserver and say 'steven' at the READY prompt - this should provide some audio feedback when recognized.

If you run festival on a different server, modify the source code in ```voiceserver.c``` and change the IP address/port values around line 260-265, recompile using 
```
gcc -o voiceserver voiceserver.c -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`
```
and rerun using ````./voiceserver -adcdev plughw:1,0 -samprate 16000 -dict /home/pi/2693.dic -lm /home/pi/2693.lm``` and confirm when saying 'steven', audio feedback is given.

### OpenHAB configuration
The OpenHab configuration consists of 2 parts. In the configuration, a TCP item needs to be added that allows for communication between OpenHAB and te RPi. Depending on what TCP addon is used, it should be something like below:

```java
String rpi { tcp="*[192.168.1.130:3000]" } // raspberry pi ip address and port (hardcoded to 3000), both directions
```

Then a set of rules need to be added: 1 to initiate the connection between the RPi and 1 to handle incoming commands that are send from the RPi and handled by OpenHAB. See exampels below (note that this might also depend on the TCP addon version, what should be used). Basically, when the state of the TCP item changes (i.e. a message is send from the RPI to OpenHAB), something needs to be executed by OpenHAB. When a command is send form OpenHAB to the RPi, it will (via festival) provide the audio feedback. If voiceserver is run form an SSH session, this in/out flow should be visible in the terminal.

```java
// Startup rule
rule Startup
when
   System started
then
   // let the voiceserver know that this is openhab by sending this string
 	 sendCommand(rpi, "OPENHAB")
end


rule rpi_command_received
when
	 Item rpi changed
then
	 // logic to handle incoming command
	 // print("Incoming:\n")
	 // sendCommand(Bedpi, "Response from openhab")
	 print("rpi:" + rpi.state.toString + "\n")
	 
 	// check for lights on
 	if (rpi.state.toString.containsIgnoreCase("lights")) {
  		println("lights command...")
		  if (rpi.state.toString.containsIgnoreCase("on")) {
			   // due something with the lights and send a response
			   rpi.sendCommand("Turning lights on");
		  } 
		  if (rpi.state.toString.containsIgnoreCase("off")) {
			   // due something with the lights and send a response
						rpi.sendCommand("Turning lights off");
		  }
  }	

end 
```


### Missing in this setup
- Correct TCP config in OpenHAB?
- Setup WiFi (details)
- Set hostname
- Install basic stuff (e.g. nfs-client)
- Install additional voices



