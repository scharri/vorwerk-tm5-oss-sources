#!/usr/bin/env python
import sys

if len(sys.argv) < 2:
    print "You must pass 1 parameter: input .dat file"
    sys.exit (1)

fi = sys.argv[1]

s = open(fi).read()
lo = []
for idx, i in enumerate(range(0, len(s), 4)):
    # reverse each 4-byte, as .dat has raw LE memory content and we need normalized values
    word = '0x%02x%02x%02x%02x' % (ord(s[i+3]), ord(s[i+2]), ord(s[i+1]), ord(s[i]))
    print 'Word %d: %s' % (idx, word)
    lo.append (word)

so = ','.join(lo)
# write out
open('otp_bits.txt', 'w').write(so)
