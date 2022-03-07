This code uses libhackrf for a basis, and I have created an AM/FM/WBFM/SSB
demodulator with this stuff.  Additionally, AM/FM/WBFM/SSB transmit
functionality has been provided.  The goal here was to keep it light weight
so that it can run on a BeagleBone Black board.

There is no GUI, but instead, I have created a command interpreter that you
access via netcat or telnet to port 20300.  You can change modulation and
demodulaton modes on the fly, and you can change various radio parameters.

Let me mention a few things.  First of all, I did *not* write the libhackrf
code.  I've merely incorporated a snapshot of it into my code since it provides
the foundation for my system, and I want people to be able to build the code
in a standalone fashion.

Next up, I did *not* write any of the pfkutils code.  I incorporated a
snapshot of Phil's code, and I specifically use his threadslinger code to
provide a thread-to-thread messaging mechanism in my code.  Finally, I
didn't use makefiles, although, I don't really have anything against using
make.  This occurred because I wanted to whip something up quick and dirty.
The system grew though, but I still don't really have any regrets as to my
decision.  The result is that the user need only have g++, and bash to build
my code (and the other people's code which I have included in the system).

Here's how you build this stuff.

1. Clone the repository.
2. Navigate to the radioDiags directory.
3. Type "sh buildLibs.sh".
4. Type "buildRadioDiags.sh.

The executable will be in radioDiags/bin/radioDiags.

Let's talk about receiver functionality first.  The demodulated output is
presented to stdout, and the format is 8000Samples/s, 16-bit, Little Endian
PCM.  You could run the program in the following manner.

bin/radioDiags | aplay -f S16_LE -r 8000

Now that the program is running, you need to connect to the command and
control link to port 20300 via netcat or telnet (I prefer netcat).  A
welcome message will appear, and if you type help, you will be presented
with a help menu.  Okay, let's say that you want to listen to a local
station on the FM broadcast band.  At my location, I would type the following:

1. "start receiver"
2. "set frequency 91500000"
3. "set demodmode 3"

This starts the receiver up, sets the frequency to 91.5MHz, and the
demodulation mode to wideband FM.  If you type help, you'll see how to set
the other demodulator modes.  You can do other things also such as setting
the demodulator gains.

What if you don't have a sound card on the computer for which the software
is executing on?  For example, I use a BeagleBone Black to run the SDR code
(it's fast enough to do all the signal processing), and I use a 200MHz
Pentium 2 computer to play the sound?  Here's what I do.

On the BeagleBone Black, type the following (note that the -u argument to
netcat forces UDP usage.  If you don't have UDP support, don't use the -u
argument):

1. "bin/radioDiags | netcat -u <IP address of host computer> 8000"
2. Perform steps t to 3 above to listen to your local FM station on 91.5MHz.
  
On the host computer (the one with the sound card), type the following:
1. "netcat -l -u -p 8000 | aplay -f S16_LE -r 8000"
2. On a separate shell, type "netcat <IP address of BeagleBone Black> 20300"
3. Peform steps 1 to 3 above to listen to your local FM station on 91.5Mhz.
  
Let it be noted that all demodulated output is 8000Samples/s PCM (for all
demodulation modes).  I made this decision because I wanted to be able to
change demodulation modes on the fly.  Also note that steps 1 to 3 can be
performed in any order.  You can change any of the receiver parameters
without having to stop and restart the receiver.  Yes, there is a "stop
receiver" command.

Okay, let's talk about transmit functionality.  The scenario that I describe
here is to run the HackRf code on the BeagleBone Black and to use the host
 computer to send a PCM stream to the HackRF via a netcat connection.
 Here are the steps:

1. From a shell on the BeagleBone Black, type
"netcat -l -p 8000 | bin/radioDiags"
2. On the host computer, type "arecord -t raw -f S16_LE -r 8000 | netcat
<IP address of BeagleBone Black> 8000"
3. From a shell on the host computer, type "netcat <IP address of BeagleBone
Black> 20300"
4. Now that there exists a command and control link (via netcat) to the
BeagleBone Black, type the following:

4.1 "set frequency <desired frequency in Hz>"
4.2 "select livesource"
4.3 "set modmode [1|2|4|5]"
4.4 "start livestream"
4.5 "start transmitter"
  
 For example, if you want to transmit on 27.125MHz using amplitude
x modulation, you'd type:

 1. "set frequency 27125000"
 2. "set modmode 1".
 
 After carrying out the above steps, one could speak into a microphone
(connected to the sound card of the host computer) and hear themselves
on an HF receiver tune to 27.125MHz using AM demodulation.

Now, let's discuss transmission of a file of IQ data that is loaded
into mamory,  In this case, on the host computer type,

1. "load iqfile <someiqfile>"
2. "select filesource"
3. "start transmitter"

After carrying out the above steps, the file data (that was loaded into
memory) will be transmitted over and over in a cyclic manner.

Now where do you get these IQ data files?  I'm glad you asked that
question.  In each of the modulator directories there exist test
applications that accept an input file (through stdin) that contains data
to be modulated.  The format of the input data is 16-bit signed little
endian integers sampled at 8000S/s.  I use PCM audio for my use case.
Each of the modulator utilities (am, fm, wbfm, and ssb) accept the input
data and output modulated data to stdout.  The format of the output data is
8-bit signed integers sampled at 2048000S/s.  It follows that a 5 seconds
of captured PCM data will result into a file size of 2048000 bytes.  This
means that 2048000 bytes of memory must be available for the data buffer
that is used for transmission.

Now how do you build these modulator test apps?  In each of the modulator
directories (AmModulator, FmModulator, WbFmModulator, SsbModulator), there
exist shell scripts: buildAm.sh, buildFm.sh, buildWbFm.sh, and buildSsb.sh.
You merely run these scripts to create the appropriate apps: am, fm, wbfm,
and ssb.  I generally build these apps on X86 since the output files
serve as nice test vectors (for transmission) for my demodulator testing
at work.  The hackRF serves as a nice signal generator when I want to test
demodulators for reception of voice transmission.

Okay, it's time to discuss interaction with the command line interpretor. 
When the user types "help", the output appears as illustrated below.

******************** Begin Help Output ********************************

select filesource
select livesource
set demodmode <mode: [0 (None) | 1 (AM) | 2 (FM)
                      | 3 (WBFM)] | 4 (LSB) | 5 (USB)>]
set modmode <mode: [0 (None) | 1 (AM) | 2 (FM)
                      | 3 (WBFM) | 4 (LSB) | 5 (USB)>]
set amdemodgain <gain>
set fmdemodgain <gain>
set wbfmdemodgain <gain>
set ssbdemodgain <gain>
set ammodindex <modulation index>
set fmmoddeviation <deviation in Hz>
set wbfmmoddeviation <deviation in Hz>
enable rxfrontendamp
disable rxfrontendamp
enable agc
disable agc
set agctype <type: [0 (Lowpass) | 1 (Harris)]>
set agcdeadband <deadband in dB: (0 < deadband < 10)>
set aset agcalpha <alpha: (0.001 <= alpha < 0.999)>
set agclevel <level in dBFs>
enable txfrontendamp
disable txfrontendamp
set txifgain <gain in dB>
set rxifgain <gain in dB>
set rxbasebandgain <gain>
set frequency <frequency in Hertz>
set bandwidth <bandwidth in Hertz>
set samplerate <samplerate in S/s>
set warp <warp in ppm>
set squelch <threshold>
start transmitter
stop transmitter
start receiver
stop receiver
start livestream
stop livestream
set fscanvalues <startfrequency> <endfrequency> <stepsize>
start fscan
stop fscan
start frequencysweep <startfrequency> <stepsize> <count> <dwelltime>
stop frequencysweep
load iqfile <filename>
get radioinfo
get fscaninfo
get sweeperinfo
get agcinfo
exit system
help
Type <^B><enter> key sequence to repeat last command

******************** End Help Output **********************************

When the user types "get radioinfo", the output appears as illustrated
below.

******************** Begin Get Radio Info Output **********************

------------------------------------------------------
Radio Internal Information
------------------------------------------------------
Information Source                  : File
Receive Enabled                     : No
Receive Front End Amp Enabled       : Yes
Receive IF Gain:                    : 0 dB
Receive Baseband Gain:              : 0 dB
Receive Frequency                   : 89100000 Hz
Receive Bandwidth                   : 1750000 Hz
Receive Sample Rate:                : 2048000 S/s
Receive Frequency Warp:             : -30 ppm
Receive Timestamp                   : 376176640
Receive Block Count                 : 2870
Transmit Enabled                    : No
Transmit Front End Amp Enabled      : No
Transmit IF Gain:                   : 35 dB
Transmit Frequency                  : 89100000 Hz
Transmit Bandwidth                  : 1750000 Hz
Transmit Sample Rate                : 2048000 S/s
Transmit Frequency Warp:            : -30 ppm
Transmit Block Count                : 8789

--------------------------------------------
Data Consumer Internal Information
--------------------------------------------
Last Timestamp           : 376176640
Short Block Count        : 0

--------------------------------------------
Data Provider Internal Information
--------------------------------------------
IQ File Name            : /home/gianakop/HackRfModulators/wbfm.iq
IQ Sample Buffer Length : 20480000
IQ Sample Buffer Index  : 1245184

--------------------------------------------
IQ Data Processor Internal Information
--------------------------------------------
Demodulator Mode         : AM
Signal Detect Threhold   : -20 dBFs

--------------------------------------------
Baseband Data Processor Internal Information
--------------------------------------------
Stream State           : Idle
Synchronization State  : Unsynchronized
PCM Writer Index       : 14
PCM Reader Index       : 6
Buffers Produced       : 258831
Buffers Consumed       : 3549
PCM Blocks Dropped     : 109
PCM Blocks Added       : 122
Modulator Mode         : WBFM

--------------------------------------------
AM Demodulator Internal Information
--------------------------------------------
Demodulator Gain         : 300.000000

--------------------------------------------
FM Demodulator Internal Information
--------------------------------------------
Demodulator Gain         : 10185.916016

--------------------------------------------
Wideband FM Demodulator Internal Information
--------------------------------------------
Demodulator Gain         : 10185.916016

--------------------------------------------
SSB Demodulator Internal Information
--------------------------------------------
Demodulation Mode        : LSB
Demodulator Gain         : 300.000000

--------------------------------------------
AM Modulator Internal Information
--------------------------------------------
Modulator Index          : 0.800000

--------------------------------------------
FM Modulator Internal Information
--------------------------------------------
Frequency Deviation:      : 3500.000000Hz
Phase Accumulator         : -1.731567rad

--------------------------------------------
Wideband FM Modulator Internal Information
--------------------------------------------
Frequency Deviation:      : 70000.000000Hz
Phase Accumulator         : -1.104594rad

--------------------------------------------
SSB Modulator Internal Information
--------------------------------------------
Modulation Mode        : USB

******************** End Get Radio Info Output *************************

The new features I have added are listed below:

1. A signal squelch that works directly with the average magnitude of
a received IQ data block.

2. A frequency scanner that lets you specify a start frequency, an end
frequency, and a frequency step. This collaborates with the squelch
such that, when scanning, if a signal exceeds the squelch threshold,
the scan will stop so that you can hear the demodulated audio of the
signal. When the signal goes away, frequency scan will continue.

3. An automatic gain control (AGC) for the receiver.  For my purposes,
this was a much-needed feature since the HackRF doesn't provide AGC
functionality.  I wasn't a fan of always changing the system gain
parameters manually as I listened to different frequencies.  For
example, I might tune to a local weather station at one time, and tune
to a strong local FM radio station (FM broadcast band) at another time.
There can be a significant variation in signal level between these two
scenarios.  This is handled automatically by the AGC.

When the user types "get fscaninfo" (for the frequency scanner), the
output appears as illustrated below.  Not that the start frequency, the
end frequency, and the frequency increment are configurable.  See the
help command output for the syntax of the commands related to the
frequency scanner (fscan).

******************** Begin GetFscainfo Info Output **********************

--------------------------------------------
Frequency Scanner Internal Information
--------------------------------------------
Scanner State             : Scanning
Start Frequency           : 120350000 Hz
End Frequency             : 120550000 Hz
Frequency Increment       : 25000 Hz
Current Frequency         : 120475000 Hz

******************** End Get Fscainfo Output  **** **********************

When the user types "get agcinfo" (for the AGC), the output appears as
illustrated below.  Note that the algorithm type, lowpass filter
coefficient, and the operating point are user configurable.  See the
 help command output for the syntax of the commands related to the AGC.

******************** Begin Getagcinfo Info Output **********************

--------------------------------------------
AGC Internal Information
--------------------------------------------
AGC Emabled               : Yes
AGC Type                  : Harris
Lowpass Filter Coefficient: 0.800
Deadband                  : 1 dB
Operating Point           : -6 dBFs
RF Gain                   : 0 dB
IF Gain                   : 40 dB
Baseband Gain             : 49 dB
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
Signal Magnitude          : 51
RSSI (After IF Amplifier) : -56 dBFs
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

******************** End Get Agcinfo Output  **** **********************


Anyway, if you have any questions, you can always catch me on libera IRC.
I use the nick wizardyesterday.  I can also be reached on Facebook as
Chris Gianakopoulos.

Oh one last thing.  Anybody can use my software without grief.  I guess I'll
have to put the GNU open source stuff at the beginning of my files, and that
will happen later.  Either way, feel free to use my software, sell it, or
give it away.  I believe in sharing what I create to benefit the Open Source
Community.

