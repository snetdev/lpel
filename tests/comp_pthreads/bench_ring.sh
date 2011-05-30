#!/bin/bash


F_LPEL=results_ring_lpel
F_PTHR=results_ring_pthr

for i in `./space.py log 1 5 19`
do
  echo -n $i" "
  expr 1000000 / $i
done > tmp

rm -f $F_LPEL.tmp $F_PTHR.tmp
cat tmp | while read t r
do
  make clean ring "DEFINES=-DBENCHMARK -DRING_SIZE=$t -DROUNDS=$r"
  ./ringtest >> $F_LPEL.tmp
  ./pthr_ringtest >> $F_PTHR.tmp
done
rm tmp

mv $F_LPEL.tmp $F_LPEL
mv $F_PTHR.tmp $F_PTHR
