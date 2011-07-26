#!/bin/bash

PREFIX=${1:-Lpel}
TMPFILE=prefixes.tmp

nm liblpel.a | cut -d " " -f 2,3 | grep ^T | cut -d " " -f 2 > $TMPFILE

while read funcname
do
  if [[ $funcname != $PREFIX* ]]
  then
    echo $funcname
  fi
done < $TMPFILE

rm -f $TMPFILE
