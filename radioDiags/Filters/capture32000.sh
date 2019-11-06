#!/bin/sh

arecord -t raw -f S16_LE -r 32000 -d 10 $1

exit 0
