#!/bin/bash

F_LPEL=results_pipe_lpel
F_PTHR=results_pipe_pthr

place="PLACE_CONST PLACE_RR PLACE_PARTS"

lpel_files=""
for p in $place
do
  lpel_files="${lpel_files} ${F_LPEL}_$p"
done


for f in $lpel_files
do
  awk '{print $1, $3 / 1000, $2 / $3 }' <(join $F_PTHR $f) > $f.d
done


awk '{print $1, $2 / 1000, 1 }' $F_PTHR > $F_PTHR.d


TITLE="Throughput speedup for a pipeline"
gnuplot <(sed -e "s/%TITLE%/$TITLE/g"  \
              -e "s/%F_LPEL%/$F_LPEL/g"\
              -e "s/%F_PTHR%/$F_PTHR/g"\
              pipetest.p)
