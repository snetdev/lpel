#!/bin/bash

LOWER=10
UPPER=1000
SAMPLES=19

NUM_WORKERS=`grep ^processor /proc/cpuinfo | wc -l`

F_LPEL=results_pipe_lpel
F_PTHR=results_pipe_pthr

place="PLACE_CONST PLACE_RR PLACE_PARTS"

for i in `./space.py lin $LOWER $UPPER $SAMPLES`
do
  echo $i" "
done > tmp


rm -f $F_PTHR.tmp ${F_LPEL}_*.tmp
cat tmp | while read t
do
  for p in $place
  do
    make clean pipe "DEFINES=-DBENCHMARK -DNUM_WORKERS=${NUM_WORKERS} -DPIPE_DEPTH=$t -DPLACEMENT=$p"
    ./pipetest >> ${F_LPEL}_$p.tmp
  done
  ./pthr_pipetest >> $F_PTHR.tmp
done
rm tmp

for p in $place
do
  awk '{print $3, $2}' ${F_LPEL}_$p.tmp > ${F_LPEL}_$p
  rm ${F_LPEL}_$p.tmp
done

awk '{print $3, $2}' $F_PTHR.tmp > $F_PTHR
rm $F_PTHR.tmp
