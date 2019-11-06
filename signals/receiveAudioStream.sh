#!/bin/bash

# Let's accept the audio stream, modulate it, upsample it and transmit it.
netcat -l -p 8000 | ./a.out | ./interpolateSignal | hackrf_transfer -f 161670000 -s 2048000 -x 45 -C -22 -t -

exit 0
