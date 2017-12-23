#!/bin/sh
for i in $(ls | grep -e "\.cre$")
do ./build.sh ${i%????}
done
