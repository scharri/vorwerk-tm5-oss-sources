#-----------------------------------------------------------------------------
# Name:        getCh.py
# Purpose:     Common getch for Unix and DOS
#
# Copyright (c) Freescale Semiconductor, Inc.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
# o Redistributions of source code must retain the above copyright notice, this list
#   of conditions and the following disclaimer.
#     
# o Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
#    
# o Neither the name of Freescale Semiconductor, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#-----------------------------------------------------------------------------
#!/bin/env python

# API Version
import struct as _struct
import time as _time

_SUCCESS = 0
_FAIL = 1

#//////////////////////////////////////////////////////////////////////////////
#//////////////////////////////////////////////////////////////////////////////
class _Getch:
    """Get a single character from standard input.  Does not echo to the screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self): return self.impl()

class _GetchUnix:
    def __init__(self):
        import tty, sys

    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

class _GetchWindows:
    def __init__(self):
        import msvcrt

    def __call__(self):
        import msvcrt
        return msvcrt.getch()

#//////////////////////////////////////////////////////////////////////////////
# polls keyboard for character
#//////////////////////////////////////////////////////////////////////////////
def keypress(msg="Press any key to continue", spinner=False):
    """Work for Windows only"""
    try:
        import msvcrt
    except:
        print "Unit Test uses Windows call"
        return None
    getch = _Getch()
    cursor = [ '-', '\\', '|', '/']
    idx = 0
    if (msg == None) or (len(msg) == 0):
        msg = "Press any key to continue"
    print("   %s" % msg),
    while not msvcrt.kbhit():
        if spinner:
            print "\r" + cursor[idx],
        _time.sleep(0.1)
        idx += 1
        if idx > 3:
            idx = 0
    ch = getch()
    return ch


#//////////////////////////////////////////////////////////////////////////////
#//////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':
    print"\nDefault keypress..."
    keypress()
    print"\nReplace msg..."
    keypress("Press without spinner",False)
    print"\nSpinner..."
    keypress("",True)
    print"\n...Done..."

