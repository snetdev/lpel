#!/bin/bash

F_LPEL=results_ring_lpel
F_PTHR=results_ring_pthr

for f in $F_LPEL $F_PTHR
do
  awk '{print $3, ($2 / $1) / 1000.0 }' $f > $f.d
done

TITLE="Execution time per message-hop (ctx-switch)"
gnuplot <(sed -e "s/%TITLE%/$TITLE/g"  \
              -e "s/%F_LPEL%/$F_LPEL/g"\
              -e "s/%F_PTHR%/$F_PTHR/g"\
              ringtest.p)
