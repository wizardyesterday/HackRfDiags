#!/bin/sh

aplay -t raw -f S16_LE -r 16000 $1

exit 0
