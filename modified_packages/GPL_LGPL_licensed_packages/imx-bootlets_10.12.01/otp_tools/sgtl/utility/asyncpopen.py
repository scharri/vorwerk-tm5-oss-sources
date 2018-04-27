#!/bin/env python

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

## \ingroup bin
# @file asyncepopen.py
#
# Python script that allows asyncronous input into the command line.

## @package sgtl
#
# This file is primarily used for the rib python class rib.py.  It allows
# a script to interactivly use the command line.
# 

import os
import subprocess
import errno
import time
import sys

PIPE = subprocess.PIPE


from win32file import ReadFile, WriteFile
from win32pipe import PeekNamedPipe
import msvcrt


## This class extends the original class Popen in subprocess.
#
#
class Popen(subprocess.Popen):
    
    def __del__(self):
        pass
    
    ## 
    #
    def recv(self, maxsize=None):
        return self._recv('stdout', maxsize)
    
    
    ##
    #
    #
    def recvErr(self, maxsize=None):
        return self._recv('stderr', maxsize)


    ##
    #
    #
    def sendRecv(self, input='', maxsize=None):
        return self.send(input), self.recv(maxsize), self.recvErr(maxsize)


    ##
    #
    #
    def getConnMaxsize(self, which, maxsize):
        if maxsize is None:
            maxsize = 1024
        elif maxsize < 1:
            maxsize = 1
        return getattr(self, which), maxsize
    
    
    ##
    #
    #
    def _close(self, which):
        getattr(self, which).close()
        setattr(self, which, None)
    
    
    ##
    #
    #
    def send(self, input):
        if not self.stdin:
            return None

        try:
            x = msvcrt.get_osfhandle(self.stdin.fileno())
            (errCode, written) = WriteFile(x, input)
        except ValueError:
            return self._close('stdin')
        except (subprocess.pywintypes.error, Exception), why:
            if why[0] in (109, errno.ESHUTDOWN):
                return self._close('stdin')
            raise

        return written


    ##
    #
    #
    def _recv(self, which, maxsize):
        conn, maxsize = self.getConnMaxsize(which, maxsize)
        if conn is None:
            return None
        
        try:
            x = msvcrt.get_osfhandle(conn.fileno())
            (read, nAvail, nMessage) = PeekNamedPipe(x, 0)
            if maxsize < nAvail:
                nAvail = maxsize
            if nAvail > 0:
                (errCode, read) = ReadFile(x, nAvail, None)
        except ValueError:
            return self._close(which)
        except (subprocess.pywintypes.error, Exception), why:
            if why[0] in (109, errno.ESHUTDOWN):
                return self._close(which)
            raise
        
        if self.universal_newlines:
            read = self._translate_newlines(read)
        return read

    

## Default error message
#
message = "Other end disconnected!"

##
#
#
def recvSome(popenInstance, timeout=1, displayException=True, stderr=0):
    processTimeout = time.time() + timeout
    processOutput = []
    output = ""
    popenRecieve = popenInstance.recv
    if stderr:
        popenRecieve = popenInstance.recvErr
    while time.time() < processTimeout or output:
        output = popenRecieve()
        if output is None:
            if displayException:
                raise Exception(message)
            else:
                break
        elif output:
            processOutput.append(output)
        else:
            time.sleep(max((processTimeout-time.time()), 0))
    return ''.join(processOutput)
    
    
##
#
#
def sendAll(popenInstance, data):
    while len(data):
        sent = popenInstance.send(data)
        if sent is None:
            raise Exception(message)
        data = buffer(data, sent)



#//////////////////////////////////////////////////////////////////////////////
# This tool cannot be used as "main"
#//////////////////////////////////////////////////////////////////////////////  
if __name__ == '__main__':
    #Example usage
    shell, commands, tail = ('cmd', ('dir /w', 'echo HELLO WORLD'), '\r\n')
  
    a = Popen(shell, stdin=PIPE, stdout=PIPE)
    print recvSome(a),
    for cmd in commands:
        sendAll(a, cmd + tail)
        print recvSome(a),
    sendAll(a, 'exit' + tail)
    print recvSome(a, e=0)
    a.wait()



