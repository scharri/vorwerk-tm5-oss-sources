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
import os
import unittest

parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)
from utility import misc

__all__ = ["BuildContext", "OperationBase", "ExternalToolOperation", "tool", "suite"]

# Abstract context class. Defines the interface for the context used by the
# operation classes below.
class BuildContext:
	def logHandler(self):
		pass
	
	def errorHandler(self):
		pass
	
	def joinAndNormalizePath(self, path, base=None):
		pass
	
# Base class for all build operations.
class OperationBase:
	def __init__(self):
		self._context = None
	
	def setContext(self, context):
		self._context = context
	
	# This is the method called by external code. It implements some common
	# error handling.
	def run(self):
		try:
			self.runInternal(self._context)
		except Exception, e:
			self.reportError(e)
		
	# This method must be overridden by subclasses to implement their
	# specific behaviour.
	def runInternal(self, context):
		pass
	
	# Calls through to the error handler set in the context.
	def reportError(self, exception):
		if self._context:
			handler = self._context.errorHandler()
			if handler:
				handler.reportError(exception)
		else:
			# default behaviour if no error handler available
			print "Error:", exception
	
	# Calls through to the log handler set in the context.
	def log(self, message):
		if self._context:
			logger = self._context.logHandler()
			if logger:
				logger.log(message)
				return
		
		# default behaviour if no logger available
		print message
	
	def joinAndNormalizePath(self, path, base=None):
		if self._context:
			return self._context.joinAndNormalizePath(path, base)
		elif base:
			return os.path.normpath(os.path.join(base, path))
		else:
			return os.path.normpath(path)

class OperationBaseUnitTest(unittest.TestCase):
	def test_base(self):
		pass

# Return a suite containing all the test cases for this module.
def suite():
	baseSuite = unittest.makeSuite(OperationBaseUnitTest)
	suite = unittest.TestSuite((baseSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
