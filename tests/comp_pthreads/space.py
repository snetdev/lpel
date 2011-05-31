#!/usr/bin/env python

import sys

if len(sys.argv) != 5:
  print "space [lin|log] <low> <high> <num>"
  exit(1)

s_type = sys.argv[1]

low = float(sys.argv[2])
high = float(sys.argv[3])
num = int(sys.argv[4])

diff = (high-low)/(num-1)

vect = map( lambda i: low+i*diff, range(num) )

for v in vect:
  if s_type=="lin": print int(v)
  elif s_type=="log": print int(10**v)
