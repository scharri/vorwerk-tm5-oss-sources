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

import sys
import unittest

__all__ = ["ErrorHandlerBase", "DefaultErrorHandler", "suite"]

class ErrorHandlerBase:
	def reportError(self, err):
		pass

class DefaultErrorHandler(ErrorHandlerBase):
	def __init__(self):
		self._logHandler = None
		self._exitOnError = False
	
	def setLogHandler(self, handler):
		self._logHandler = handler
		
	def setExitOnError(self, exit):
		self._exitOnError = exit
	
	# Tries to log the error. If no log handler has been set, it just prints
	# the error to stdout. Exits the process once the error is reported.
	def reportError(self, err):
		if self._logHandler:
			self._logHandler.log("Error: " + str(err))
		else:
			print "Error:", err
		
		if self._exitOnError:
			sys.exit(1)

# Unit test for error handler.
class ErrorHandlerUnitTest(unittest.TestCase):
	def test(self):
		import loghandler
		
		class MyLogger(loghandler.LogHandlerBase):
			def __init__(self):
				self._last = None
				
			def log(self, message, *args):
				self._last = message % args
			
			def last(self):
				return self._last
		
		logger = MyLogger()
		ehandler = DefaultErrorHandler()
		ehandler.setLogHandler(logger)
		
		e = Exception("something bad happened")
		ehandler.reportError(e)
		
		self.assertEqual(logger.last(), "Error: something bad happened")

# Return a suite containing all the test cases for this module.
def suite():
	errorHandlerSuite = unittest.makeSuite(ErrorHandlerUnitTest)
	suite = unittest.TestSuite((errorHandlerSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
