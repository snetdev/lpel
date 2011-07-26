set terminal postscript eps color solid
set autoscale
set size 0.65,0.65
#set logscale y
#set logscale x

set output "pipetest.eps"
set xlabel "Pipeline depth"
set ylabel "Execution time (sec)"
set title  "%TITLE%"
plot   "%F_PTHR%.d" using 1:2 title 'pthread' with linespoints linewidth 5,\
       "%F_LPEL%_PLACE_CONST.d" using 1:2 title 'lpel-const' with linespoints linewidth 4,\
       "%F_LPEL%_PLACE_RR.d" using 1:2 title 'lpel-rr' with linespoints linewidth 4,\
       "%F_LPEL%_PLACE_PARTS.d" using 1:2 title 'lpel-parts' with linespoints linewidth 4

