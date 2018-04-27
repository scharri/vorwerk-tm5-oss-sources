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
import controller
import operations

parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)
from utility import misc

# The subprocess package is available only in Python 2.4 and later.
try:
	import subprocess
except:
	print "subprocess package is not available: Python 2.4 or greater is required"
	sys.exit(1)

__all__ = ["ExternalToolOperation", "tool", "suite"]

# Both a base class for all operation subclasses that call out to external
# tools, and an operation in its own right. This class manages the redirection
# of I/O, formatting of arguments, and retrieving the exit status code from the
# child process.  Output is filtered by indicating keywords that output lines
# should not contain.
class ExternalToolOperation(operations.OperationBase):
	def __init__(self):
		operations.OperationBase.__init__(self)
		self._cmd = None
		self._args = []
		self._lineFilters = ["Building", "Compiling", "C++ prelinker:"]
		self._filterOutput = False
		self._showProgress = False
	
	def addArgument(self, arg):
		self._args.append(arg)
		return self
	
	def addLineFilter(self, filter):
		self._lineFilters.append(filter)
		return self
	
	def setCommandPath(self, path):
		self._cmd = path
		return self
	
	def getCommandPath(self):
		return self._cmd
	
	def setShowProgress(self):
		self._showProgress = True
		return self
		
	def outputFilter (self, line):
		outputLine = True
		
		for filter in self._lineFilters:
			if line.startswith(filter):
				outputLine = False
				break
				
		return outputLine
	
	# formats the command for shell execution
	def formatCommand(self):
		cmd = self.joinAndNormalizePath(self._cmd)
		for arg in self._args:
			cmd += misc.mymkarg(arg)
		return cmd
	
	# Returns a subprocess.Popen object.
	def executeCommand(self):
		cmd = self.formatCommand()
		self.log("Running command: " + cmd + os.linesep)
		
		# prefix args list with the executable name
		args = [self._cmd] + self._args
		
		# we must use shell on win32 if the command isn't a full path. unix
		# never uses the shell.
		useShell = not os.path.isabs(self._cmd)
		if sys.platform != 'win32':
			useShell = False

		# create line buffered child process
		child = subprocess.Popen(args, bufsize=1, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=useShell)
		return child

	def copyOutputToLog(self, child):
		prevOutputLine = True
		prevOutputBuffer = ""
		
		# Copy child's output to log.
		while True:
			returncode = child.poll()

			line = child.stdout.readline()
			
			# Exit this loop if we are at the end of output and the process has exited.
			# If the child is not finished, keep trying to read output and write it to
			# the log.
			if not line and returncode != None:
				break
			elif line:
				#Replace any percent signs that can cause an error when outputted to log
				line = line.replace(r"%","(Percent Sign)")
			
				
				if not self._showProgress:
					#User filters to decide which lines are actually printed
					outputLine = self.outputFilter(line)
					if outputLine:
						if not prevOutputLine:
							self.log("\n")
							self.log(prevOutputBuffer)
							
							
						self.log(line)
						
						prevOutputLine = True
					else:
						#Dots are only printed to screen, not logged
						prevOutputBuffer = line
						sys.stdout.write(".")
						prevOutputLine = False
				else:
					self.log(line)
				
				
		
		return returncode
	
	def runInternal(self, context):
		self.executeCommand()

# Convenience function to create an external tool operation.
def tool(cmd, args):
	op = ExternalToolOperation()
	op.setContext(controller.buildContext)
	op.setCommandPath(cmd)
	for arg in args:
		op.addArgument(arg)
	return op

# Unit test for ExternalToolOperation class.
class ExternalToolOperationUnitTest(unittest.TestCase):
	def test_format(self):
		op = ExternalToolOperation()
		op.setCommandPath("echo")
		op.addArgument("yes")
		op.addArgument("no")
		self.assertEqual(op.formatCommand(), r'echo yes no')
		
	def test_execute(self):
		op = ExternalToolOperation()
		op.setCommandPath("echo")
		op.addArgument("yes")
		op.addArgument("no")
		child = op.executeCommand()
		child.wait()
		result = child.returncode
		out = child.stdout
		self.assertEqual(result, 0)
		self.assertEqual("yes no" + os.linesep, out.read())

# Return a suite containing all the test cases for this module.
def suite():
	externalToolSuite = unittest.makeSuite(ExternalToolOperationUnitTest)
	suite = unittest.TestSuite((externalToolSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
