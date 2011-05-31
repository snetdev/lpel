set terminal postscript eps color solid
set autoscale
#set logscale y
#set logscale x

set output "pipetest.eps"
set xlabel "Pipeline depth"
set ylabel "Speedup"
set title  "%TITLE%"
plot   "%F_PTHR%.d" using 1:2 title 'pthread' with linespoints linewidth 2,\
       "%F_LPEL%_PLACE_CONST.d" using 1:2 title 'lpel-const' with linespoints,\
       "%F_LPEL%_PLACE_RR.d" using 1:2 title 'lpel-rr' with linespoints,\
       "%F_LPEL%_PLACE_PARTS.d" using 1:2 title 'lpel-parts' with linespoints

