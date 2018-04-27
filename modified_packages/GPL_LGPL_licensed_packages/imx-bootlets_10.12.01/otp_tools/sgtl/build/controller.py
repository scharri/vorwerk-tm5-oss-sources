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

import os
import operations
import unittest

__all__ = ["buildContext", "BuildController", "suite"]

buildContext = None

# Simple controller/glue class that ties all the other pieces of the
# auto-build package together.
class BuildController(operations.BuildContext):
	def __init__(self):
		self._base = os.getcwd()
		self._logHandler = None
		self._errorHandler = None
	
	def baseDirectory(self):
		return self._base
	
	def setBaseDirectory(self, path):
		self._base = path
	
	def context(self):
		return self
	
	def logHandler(self):
		return self._logHandler
	
	def setLogHandler(self, handler):
		self._logHandler = handler
	
	def errorHandler(self):
		return self._errorHandler
		
	def setErrorHandler(self, handler):
		self._errorHandler = handler
		
	def joinAndNormalizePath(self, path, base=None):
		#print "joinAndNormalizePath("+path+", "+str(base)+")"
		if os.path.isabs(path):
			return os.path.normpath(path)
		elif base != None:
			return os.path.normpath(os.path.join(base, path))
		else:
			#print "joinging "+self._baseDir+" and " +path
			return os.path.normpath(os.path.join(self._base, path))

# Unit test for move operation.
class BuildControllerUnitTest(unittest.TestCase):
	def test_basics(self):
		c = BuildController()
		self.assertEqual(c.baseDirectory(), os.getcwd())
		c.setBaseDirectory('..')
		self.assertEqual(c.baseDirectory(), '..')
		self.assertEqual(c.context(), c)
		self.assertEqual(c.logHandler(), None)
		self.assertEqual(c.errorHandler(), None)
	
	def test_base_dir(self):
		c = BuildController()
		
		c.setBaseDirectory('.')
		self.assertEqual(c.joinAndNormalizePath('.'), '.')
		self.assertEqual(c.joinAndNormalizePath('foo'), 'foo')
		
		c.setBaseDirectory('..')
		self.assertEqual(c.joinAndNormalizePath('foo'), os.path.join('..', 'foo'))
		self.assertEqual(c.joinAndNormalizePath(os.path.join('foo', 'bar', 'baz')), os.path.join('..', 'foo', 'bar', 'baz'))
		
		self.assertEqual(c.joinAndNormalizePath(os.getcwd()), os.getcwd())
		
		self.assertEqual(c.joinAndNormalizePath('foo', 'bar'), os.path.join('bar', 'foo'))
		self.assertEqual(c.joinAndNormalizePath('foo', os.path.join('bar', 'baz', 'buzz')), os.path.join('bar', 'baz', 'buzz', 'foo'))

# Return a suite containing all the test cases for this module.
def suite():
	controllerSuite = unittest.makeSuite(BuildControllerUnitTest)
	suite = unittest.TestSuite((controllerSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
