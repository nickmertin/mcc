#!/bin/sh
echo Building test $1...
if ! ../cmake-build-debug/mcc-cre-linux-x86 --pipe < $1.cre > $1.S
then echo ERROR: $?; exit $?
fi
if [[ $(uname -m) != "i?86" ]]
then echo Not on x86 platform, cannot assemble
elif ! as $1.S -o $1.o
then echo ERROR: $?; exit $?
else echo Success
fi
