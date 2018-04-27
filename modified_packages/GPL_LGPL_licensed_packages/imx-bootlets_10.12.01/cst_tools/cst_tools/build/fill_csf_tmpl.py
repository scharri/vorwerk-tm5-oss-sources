#!/usr/bin/env python
import sys
import os
import re

if len(sys.argv) < 2:
    print "You must pass 1 parameters: input .tmpl file"
    sys.exit (1)

fi = sys.argv[1]

s = open(fi).read()
m = re.search ('.*Blocks\s*=\s*.*"(.*)"', s)
if not m:
    print "Filename not found!"
    sys.exit (1)

filename = m.groups()[0]
filesize = os.path.getsize (filename)
if filename.endswith ('linux_kernel.bin'):
    rest = filesize % 64
    if rest != 0:
	# pad size to 64 bytes, required for byte streams that are not last in list
	#filesize = filesize + 64 - rest
	# however it seems that we cannot exceed real file size, so instead we will calculate signature excluding file tail
	filesize -= rest
s = s.replace ('{XXXX}', '0x%x' % filesize)

# additional handling for linux_kernel.csf
m = re.search ('.*"(.*zImage.*)"', s)
if m:
    filename = m.groups()[0]
    filesize = os.path.getsize (filename)
    s = s.replace ('{YYYY}', '0x%x' % filesize)


# write out
##print s
fo = fi.rstrip('.tmpl')
open(fo, 'w').write(s)
