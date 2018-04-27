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
import types
import operations

# this is a little hack to let python find a sibling package to us
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)
import versions.versionedfile

__all__ = ["VersioningOperation", "setVersion", "suite"]

class VersioningOperation(operations.OperationBase):
	"""Operation subclass to set the version of one or more .sb or resource files.
	If a file does not have a signature of 'SGTL' or 'RSRC' it will not be
	modified and an error will be reported. 
	
	Accepts either 3-tuple or string (in x.y.z format) for versions. At
	least one file must be provided, and at least one of the product or
	component version must be specified.
	
	The file signatures cannot be changed using this operation."""
	
	def __init__(self, files=None, product=None, component=None):
		"""Accepts either a string or list of strings for the files argument."""
		operations.OperationBase.__init__(self)
		
		# accept either a string or list
		if type(files) == types.ListType:
			self._files = files
		elif not files is None:
			self._files = [files]
		else:
			self._files = []
		
		if not product is None:
			self._product = self.convertVersion(product)
		else:
			self._product = None
		
		if not component is None:
			self._component = self.convertVersion(component)
		else:
			self._component = None
	
	def setFile(self, path):
		self._files = [path]
	
	def addFile(self, path):
		self._files.append(path)
	
	def setProductVersion(self, version):
		self._product = self.convertVersion(version)
	
	def setComponentVersion(self, version):
		self._component = self.convertVersion(version)
		
	def parseVersion(self, versionString):
		"""Returns a tuple containing the integers separated by periods in the
		versionString argument."""
		return tuple([int(x) for x in versionString.split('.')])
	
	def convertVersion(self, version):
		"""Returns a 3-tuple version. If the version argument is a string, it
		is converted using parseVersion(). If there are more than 3 elements,
		they are truncated. If there are few than 3 elements, additional
		values of 999 are added."""
		if type(version) == types.StringType:
			result = self.parseVersion(version)
		else:
			result = tuple(version)
		
		# make sure there are 3 elements in the resulting tuple
		if len(result) > 3:
			result = result[:3]
		elif len(result) < 3:
			result = result + (999,) * (3 - len(result))
		
		return result
	
	def runInternal(self, context):
		# check inputs
		if len(self._files) == 0:
			raise Exception("no files provided", self)
		if self._product is None and self._component is None:
			raise Exception("both the product and component version were not provided", self)
			
		# set version in all files
		for thisFile in self._files:
			self.processOneFile(thisFile)
		
	def processOneFile(self, path):
		realPath = self.joinAndNormalizePath(path)
		versionedFile = versions.versionedfile.VersionedFile(realPath, 'r+b')
		try:
			# make sure this is a file we recognise, either a .rsc or .sb file
			sig = versionedFile.readSignature()
			if not sig in ['RSRC', 'STMP']:
				raise Exception("unrecognized file signature '%s'" % sig, self)
			
			if self._product:
				versionedFile.writeProductVersion(self._product)
			if self._component:
				versionedFile.writeComponentVersion(self._component)
		finally:
			versionedFile.close()

def setVersion(files=None, product=None, component=None):
	"""Shorthand function to create versioning operation object.
	
	Accepts either a file path or a list of paths for the files argument. The product
	and component arguments are both optional and take versions. A version can be
	either a 3-tuple (actually any sequence object) or a string in the format 'x.y.z'.
	If either the version has fewer than 3 elements, the missing ones will
	take the value 999."""
	op = VersioningOperation(files, product=product, component=component)
	op.setContext(controller.buildContext)
	return op

# Return a suite containing all the test cases for this module.
def suite():
	# Unit test for move operation.
	class VersioningOperationUnitTest(unittest.TestCase):
		def setUp(self):
			# copy the test file so we can modify it safely
			pass
		
		def tearDown(self):
			# remove the copy of the test file
			pass
		
		def test_quick_set_version(self):
			#op = setVersion(os.path.join('vectors', 'test.rsc'), '1.2.3', '4.5.6')
			#op.run()
			pass
	
	versioningSuite = unittest.makeSuite(VersioningOperationUnitTest)
	suite = unittest.TestSuite((versioningSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	result = unittest.TextTestRunner(verbosity=2).run(suite())
	if len(result.errors) or len(result.failures):
		sys.exit(1)
