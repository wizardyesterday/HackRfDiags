# HackRfDiags
This code uses libhackrf for a basis, and I have created an AM/FM/WBFM/SSB demodulator with this stuff.  Additionally,
AM/FM/SSB (not wideband FM) transmit functionality has been provided.  The goal here was to keep it light weight so that it
can run on a BeagleBone Black board.  There is no GUI, but instead, I have created a command interpreter that you access
via netcat or telnet to port 2030.  You can change modulation and demodulaton modes on the fly, and you can change various
radio parameters.
Let me mention a few things.  First of all, I did *not* write the libhackrf code.  I've merely incorporated a snapshot of it
into my code since it provides the foundation for my system, and I want people to be able to build the code in a standalone
fashion.  Next up, I did *not* write any of the pfkutils code.  I incorporated a snapshot of Phil's code, and I specifically
use his threadslinger code to provide a thread-to-thread messaging mechanism in my code.  Finally, I didn't use makefiles,
although, I don't really have anything against using make.  This occurred because I wanted to whip something up quick and
dirty. The system grew though, but I still don't really have any regrets as to my decision.  The result is that the user
need only have g++, and bash to build my code (and the other people's code which I have included in the system).
Here's how you build this stuff.

1. Clone the repository.
2. Navigate to the radioDiags directory.
3. Type "sh buildLibs.sh".
4. Type "buildRadioDiags.sh.

The executable will be in radioDiags/bin/radioDiags.

Let's talk about receiver functionality first.
The demodulated output is presented to stdout, and the format is 8000Samples/s, 16-bit, Little Endian PCM.  You could
run the program in the following manner.

bin/radioDiags | aplay -f S16_LE -r 8000

Now that the program is running, you need to connect to the command and control link to port 20300 via netcat or telnet
(I prefer netcat).  A welcome message will appear, and if you type help, you will be presented with a help menu.  Okay, let's
say that you want to listen to a local station on the FM broadcast band.  At my location, I would type the following:

1. "start receiver"
2. "set frequency 91500000"
3. "set demodmode 3"

This starts the receiver up, sets the frequency to 91.5MHz, and the demodulation mode to wideband FM.  If you type help,
you'll see how to set the other demodulator modes.  You can do other things also such as setting the demodulator gains.

What if you don't have a sound card on the computer for which the software is executing on?  For example, I use a BeagleBone
Black to run the SDR code (it's fast enough to do all the signal processing), and I use a 200MHz Pentium 2 computer to
play the sound?  Here's what I do.

On the BeagleBone Black, type the following (note that the -u argument to netcat
forces UDP usage.  If you don't have UDP support, don't use the -u argument):
1. "bin/radioDiags | netcat -u <IP address of host computer> 8000"
2. Perform steps t to 3 above to listen to your local FM station on 91.5MHz.
  
On the host computer (the one with the sound card), type the following:
1. "netcat -l -u p- 8000 | aplay -f S16_LE -r 8000"
2. On a separate shell, type "netcat <IP address of BeagleBone Black> 20300.
3. Peform steps 1 to 3 above to listen to your local FM station on 91.5Mhz.
  
Let it be noted that all demodulated output is 8000Samples/s PCM (for all demodulation modes).  I made this decision
because I wanted to be able to change demodulation modes on the fly.  Also note that steps 1 to 3 can be performed in
any order.  You can change any of the receiver parameters without having to stop and restart the receiver.  Yes, there is
a "stop receiver" command.

Okay, let's talk about transmit functionality.
TO DO... I'll get to this later on.

Anyway, if you have any questions, you can always catch me on freenode IRC.  I use the nick wizardyesterday or adhoc_rf_rocks.

Oh one last thing.  Anybody can use my software without grief.  I guess I'll have to put the GNU open source stuff at the
beginning of my files, and that will happen later.  Either way, feel free to use my software, sell it, or give it away.  I
believe in sharing what I create to benefit the Open Source Community.

