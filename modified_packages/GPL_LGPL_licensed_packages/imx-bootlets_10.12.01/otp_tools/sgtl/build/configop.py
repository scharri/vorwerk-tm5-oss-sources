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

import operations
import unittest
import sys
import os
import controller

# this is a little hack to let python find a sibling package to us
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)
import config as configapi # we have a local function named config below

__all__ = ["ConfigOperation", "config", "suite"]

# Operation class to call into the configuration management module.
class ConfigOperation(operations.OperationBase):
	def __init__(self, templatePath, outputPath=None):
		operations.OperationBase.__init__(self)
		self._templatePath = templatePath
		self._outputPath = outputPath
		self._selected = []
		self._deselected = []
		self._properties = {}
		self._overwrite = "yes"

		"""
		If _loggingLevel is not None, then it is passed to the ECController in ConfigOperation.runInternal().
		The ECController nominally uses the "logging" module of python.  That module
		uses logging levels such as these:
			CRITICAL = 50
			FATAL = CRITICAL
			ERROR = 40
			WARNING = 30
			WARN = WARNING
			INFO = 20
			DEBUG = 10
			NOTSET = 0
		Therefore, these are the choices for _loggingLevel.
		"""
		self._loggingLevel = None
		self.bLogTheConfig = False
		
	def selectConfigurations(self, configs):
		for name in configs:
			self._selected.append(name)
		return self
	
	def deselectConfigurations(self, configs):
		for name in configs:
			self._deselected.append(name)
		return self
	
	def setOutputPath(self, path):
		self._outputPath = path
	
	def setOverwrite(self, overwriteMode):
		""" Valid values of overwriteMode are 'no', 'yes', and 'update' """
		#if overwriteMode.lower() not in ["no", "yes", "update"]:
		#	raise Exception("Invalid overwrite mode " + overwriteMode )

		self._overwrite = overwriteMode.lower()
	
	def setLoggingLevel(self, loggingLevel):
		self._loggingLevel = loggingLevel

	def setProperties(self, properties):
		for key in properties.keys():
		  self._properties[key] = properties[key]
		return self
	
	def runInternal(self, context):
		assert self._outputPath
		
		self._controller = configapi.controller.ECController(self.joinAndNormalizePath(self._templatePath))
		self._controller.setConfigValues(self._selected, True)
		self._controller.setConfigValues(self._deselected, False)
		self._controller.setPropertyValues(self._properties)
		self._controller.setOverwrite(self._overwrite)

		# If our loggingLevel has been set, then pass this setting to the ECController.
		if self._loggingLevel:
			self._controller.setLoggingLevel( self._loggingLevel )
		
		# Indicate whether or not we want to log the configuration that is generated.
		self._controller.setBLogTheConfig( self.bLogTheConfig )

		normalizedOutputPath = self.joinAndNormalizePath(self._outputPath)
		self._controller.setOutputFilePath(normalizedOutputPath)
		self._controller.run()
			

# Convenience function to create a config operation.
def config(templatePath, outputPath, configs=None, deselectedConfigs=None, properties=None, overwrite="yes", loggingLevel=None, bLogTheConfig=False):
	op = ConfigOperation(templatePath, outputPath)
	op.setContext(controller.buildContext)
	if configs:
		op.selectConfigurations(configs)
	if deselectedConfigs:
		op.deselectConfigurations(deselectedConfigs)
	op.setOverwrite(overwrite)
	if properties:
		op.setProperties(properties)
	if loggingLevel:
		op.setLoggingLevel(loggingLevel)
	op.bLogTheConfig = bLogTheConfig

	# Run "op" (which generates the configured file), and return the result.

	return op

# Unit test for the ConfigOperation class.
class ConfigOperationUnitTest(unittest.TestCase):
	def setUp(self):
		self.templatePath = os.path.join("..", "config", "vectors", "small.tgpj")
		self.outputPath = os.path.join("..", "config", "vectors", "outtest.gpj")
		
	# This test only verifies that nothing crashes and that no exceptions are
	# raised. It doesn't check to see if the configs actually work right. But,
	# that's really the territory of the config package's unit tests.
	def test_run_config(self):
		op = ConfigOperation(self.templatePath, self.outputPath)
		op.selectConfigurations(["DEBUG"])
		op.run()
	
	def test_run_quick_config(self):
		op = config(self.templatePath, self.outputPath, configs=["MMC"])
		op.run()

# Return a suite containing all the test cases for this module.
def suite():
	configSuite = unittest.makeSuite(ConfigOperationUnitTest)
	suite = unittest.TestSuite((configSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
