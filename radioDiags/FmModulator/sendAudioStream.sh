#!/bin/bash

# Let's send the audio stream.
arecord -t raw -f S16_LE -r 8000 | netcat $1 8000

exit 0
