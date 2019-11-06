#!/bin/bash

# Let's accept the audio stream, modulate it, upsample it and transmit it.
netcat -l -p 8000 | ./bb | hackrf_transfer -f 160890000 -s 2048000 -x 45 -C -22 -t -

exit 0
