#!/bin/sh

arecord -t raw -f S16_LE -r 8000 -d 5 $1

exit 0
