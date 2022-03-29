This directory contains files related to AGC performance on the HackRF
radio.
I have copied a paper, by Harris and Smith, that describes one of the
AGC algorithms that I partially implemented.  The HackRF schematic is
included for reference.

03/28/2022
Here's our average performance measurements:
Time (IQ Data Block Arrival to AGC callback invocation): 48ms.
Time (Adjust IF gain): 1ms.

The total time always is 49ms. This means that 34ms of a 64ms IQ data
block has already arrived.  The solution is to adjust the receiver gain
and skip signal evaluation of the polluted IQ data block.  This is
accomplished by setting the AGC blank value to 1.  This will mitigate the
effect of the AGC "fishtailing" due to the inherent latency of data transfer
through the radio appliction.
The latency is due to:
1. Latency through the thread-to-thread queueing system.
2. Latency due to decimating form 2.048x10^6 S/s down 256000 S/s.
3. The Linux thread scheduler.
4. Sending of Vendor Request packets via libusb (very low latency).



