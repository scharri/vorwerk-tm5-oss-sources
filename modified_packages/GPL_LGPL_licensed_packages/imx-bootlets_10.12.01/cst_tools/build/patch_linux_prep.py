#!/usr/bin/env python
import sys

if len(sys.argv) < 3:
    print "You must pass 2 parameters: input file and output file"
    sys.exit (1)

fi = sys.argv[1]
fo = sys.argv[2]

l = list(open(fi).read())
# change entry_count 0->1 (long, little endian, so this is a first byte)
l [0x24] = '\x01'
open(fo, 'w').write(''.join(l))
