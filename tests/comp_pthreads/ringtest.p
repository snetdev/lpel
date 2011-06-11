set terminal postscript eps color solid
set autoscale y
#set logscale y
set logscale x

set output "ringtest.eps"
set xlabel "Ring size"
set ylabel "ET (us)"
set title  "%TITLE%"
plot   "%F_PTHR%.d" using 1:2 title 'pthread' with linespoints linewidth 6,\
       "%F_LPEL%.d" using 1:2 title 'lpel' with linespoints linewidth 6

