#!/bin/sh

./a.out < count.raw | ./interpolateSignal > $1.iq

exit 0
