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
import externaltool
import unittest
import controller

# this is a little hack to let python find a sibling package to us
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)
import utility.misc

__all__ = ["DoxygenOperation", "doxygen", "suite"]

class DoxygenOperation(externaltool.ExternalToolOperation):
	"""Operation subclass to call doxygen."""
	
	def __init__(self, configFilePath=None, workingDirectory=None):
		externaltool.ExternalToolOperation.__init__(self)
		self._configFilePath = configFilePath
		self._workingDir = workingDirectory
	
	def setConfigurationFile(self, path):
		self._configFilePath = path
	
	def getConfigurationFile(self):
		return self._configFilePath
	
	def setWorkingDirectory(self, path):
		self._workingDir = path
		
	def getWorkingDirectory(self):
		return self._workingDir
		
	def runInternal(self, context):
		if not self._configFilePath:
			raise Exception("No doxygen configuration provided")
		
		configPath = self.joinAndNormalizePath(self._configFilePath)
		if self._workingDir:
			workingDir = self.joinAndNormalizePath(self._workingDir)
		else:
			workingDir = None
		
		self.setCommandPath("doxygen")
		self.addArgument(configPath)
		
		# save old wd
		try:
			if workingDir:
				saveWD = os.getcwd()
				os.chdir(workingDir)
				print "cwd:", os.getcwd()
			
			returncode = self.copyOutputToLog(self.executeCommand())
		finally:
			# restore old wd
			if workingDir:
				os.chdir(saveWD)

		# Raise exception if the build failed.
		if returncode != 0:
			raise Exception("doxygen failed with return code " + str(returncode))

def doxygen(configFile=None, workingDirectory=None):
	op = DoxygenOperation(configFile, workingDirectory)
	op.setContext(controller.buildContext)
	return op

# Return a suite containing all the test cases for this module.
def suite():
	# Unit test for move operation.
	class DoxygenOperationUnitTest(unittest.TestCase):
		def setUp(self):
			self.configFile = os.path.join('..', '..', '..', '_docs', 'project_files', 'development')
			self.workingDir = os.path.join('..', '..', '..', '_docs', 'project_files')
		
		def test_quick_build(self):
			op = doxygen('development', self.workingDir)
			#if sys.platform == 'win32':
			op.run()
	
	doxygenSuite = unittest.makeSuite(DoxygenOperationUnitTest)
	suite = unittest.TestSuite((doxygenSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	result = unittest.TextTestRunner(verbosity=2).run(suite())
	if len(result.errors) or len(result.failures):
		sys.exit(1)
