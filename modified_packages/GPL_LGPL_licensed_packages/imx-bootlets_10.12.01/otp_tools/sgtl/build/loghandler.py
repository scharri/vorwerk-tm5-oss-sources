#!/usr/bin/env python

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

import sys, os
import logging
import unittest

# this is a little hack to let python find a sibling package to us
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
    sys.path.append(parentModulePath)
from utility.bufferedstream import BufferedStringCharacterStream

__all__ = ["LogHandlerBase", "DefaultLogHandler", "suite"]

class LogHandlerBase:
    def log(self, message, *args):
        pass
        
class DefaultLogHandler(LogHandlerBase):
    def __init__(self):
        self._outputs = []
        self._logger = logging.getLogger()
        self._sep = os.linesep
        self._autoLineEnd = True
    
    def addOutputStream(self, stream):
        self._outputs.append(stream)
    
    def closeOutputStream(self, stream):
        for output in self._outputs:
            if output == stream:
                output.close()

    def addOutputFile(self, path, mode="w"):
        self._outputs.append(file(path, mode))
    
    def closeOutputFile(self, path):
        for output in self._outputs:
            if output.name == path:
                output.close()
                self._outputs.remove(output)

    def setLineSeparator(self, sep):
        self._sep = sep
    
    def setAutoLineEnd(self, auto):
        self._autoLineEnd = auto
        
    def resetOutputs(self):
        self._outputs = []
        
    def log(self, message, *args):
        logEntry = self.filterLineEndingsAndNulls(message % args)
        if not logEntry.endswith(self._sep) and self._autoLineEnd:
            logEntry += self._sep
        for output in self._outputs:
            output.write(logEntry)
            output.flush()
    
    def logThisFile(self, fileName, mode='r' ):
        try:
            f = file( fileName, mode )
        except:
            self.log("Failed to open file " + fileName)
            raise

        for m in f.readlines():
            self.log(m)

        f.close()


    def filterLineEndingsAndNulls(self, message):
        """Transforms all line endings in message, of any type, to self._sep.
        This defaults to os.linesep, but can be changed with the setLineSeparator()
        method.
        As a bonus, this function also removes any null characters.
        """
        result = ''
        stream = BufferedStringCharacterStream(message)
        while True:
            c = stream.get()
            if c is None:
                break
            
            if c in ['\r', '\n']:
                if c == '\r' and stream.peek() == '\n':
                    stream.get() # skip LF in CRLF pair
                result += self._sep
            elif c != '\x00':    # Omit null characters.
                result += c

        return result

# Unit test for log handler.
class LogHandlerUnitTest(unittest.TestCase):
    def test(self):
        pass

# Return a suite containing all the test cases for this module.
def suite():
    logHandlerSuite = unittest.makeSuite(LogHandlerUnitTest)
    suite = unittest.TestSuite((logHandlerSuite))
    return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite())
