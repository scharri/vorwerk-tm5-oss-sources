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
import operations
import shutil
import unittest
import controller
import types

# this is a little hack to let python find a sibling package to us
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)
import utility.filetools

__all__ = ["CopyOperation", "MoveOperation", "DeleteOperation", "copy", "move", "delete", "suite"]

class FileOperation(operations.OperationBase):
	def __init__(self):
		operations.OperationBase.__init__(self)
		self._toPath = None
		self._fromPaths = []
		
	def setToPath(self, path):
		self._toPath = path
		return self
	
	def addFromPath(self, path):
		self._fromPaths.append(path)
		return self

class CopyOperation(FileOperation):
	def __init__(self):
		FileOperation.__init__(self)
		self._mode = None
	
	def setMode(self, mode):
		self._mode = mode
		return self
	
	def runInternal(self, context):
		realDest = self.joinAndNormalizePath(self._toPath)
		if os.path.isfile(realDest):
			raise Exception("destination " + realDest + " is a file", self)
		elif not os.path.exists(realDest):
			os.makedirs(realDest)
		
		for src in self._fromPaths:
			realSrc = self.joinAndNormalizePath(src)
			if os.path.isdir(realSrc):
				# directory copies need to include the source dir name in the
				# destination location
				utility.filetools.copytree(realSrc, os.path.join(realDest, os.path.basename(realSrc)), mode=self._mode)
			else:
				utility.filetools.copy2(realSrc, realDest)
				if self._mode != None:
					os.chmod(realDest, self._mode)

class MoveOpertion(FileOperation):
	def runInternal(self, context):
		realDest = context.joinAndNormalizePath(self._toPath)
		if os.path.isfile(realDest):
			raise Exception("destination " + realDest + " is a file", self)
		elif not os.path.exists(realDest):
			os.makedirs(realDest)
		
		for src in self._fromPaths:
			realSrc = context.joinAndNormalizePath(src)
			shutil.move(realSrc, realDest)

class DeleteOperation(operations.OperationBase):
	def __init__(self):
		operations.OperationBase.__init__(self)
		self._paths = []
		self._force = False
	
	def addPath(self, path):
		self._paths.append(path)
		return self
	
	def setForce(self, forceIt):
		self._force = forceIt
		return self
	
	def runInternal(self, context):
		for path in self._paths:
			realPath = context.joinAndNormalizePath(path)
			if os.path.exists(realPath):
				if os.path.isfile(realPath):
					os.remove(realPath)
				else:
					utility.filetools.rmtree(realPath, force=self._force)
				self.log("Deleted " + path)

class RenameOperation(operations.OperationBase):
	def __init__(self):
		operations.OperationBase.__init__(self)
		self._originalPath = None
		self._newName = None
	
	def setOriginalPath(self, path):
		self._originalPath = path
	
	def setNewName(self, name):
		self._newName = name
	
	def runInternal(self, context):
		if not self._originalpath or not self._newName:
			return
		
		realOriginalPath = context.joinAndNormalizePath(self._originalPath)
		if not os.path.exists(realOriginalPath):
			raise Exception("original file '" + realOriginalPath + "' does not exist", self)
		
		realOriginalDir = os.path.dirname(realOriginalPath)
		newOriginalPath = os.path.join(realOriginalDir, self._newName)
		
		os.rename(realOriginalPath, newOriginalPath)

# Utility function to quickly create a copy operation.
def copy(toPath, fromPaths):
	op = CopyOperation()
	op.setContext(controller.buildContext)
	op.setToPath(toPath)
	if type(fromPaths) == types.ListType:
		for path in fromPaths:
			op.addFromPath(path)
	else:
		op.addFromPath(fromPaths)
	return op

# Utility function to quickly create a move operation.
def move(toPath, fromPaths):
	op = MoveOperation()
	op.setContext(controller.buildContext)
	op.setToPath(toPath)
	if type(fromPaths) == types.ListType:
		for path in fromPaths:
			op.addFromPath(path)
	else:
		op.addFromPath(fromPaths)
	return op

# Utility function to quickly create a delete operation.
def delete(paths):
	op = DeleteOperation()
	op.setContext(controller.buildContext)
	if type(paths) == types.ListType:
		for p in paths:
			op.addPath(p)
	else:
		op.addPath(paths)
	return op

# Utility function to quickly create a rename operation.
def rename(originalPath, newName):
	op = RenameOperation()
	op.setContext(controller.buildContext)
	op.setOriginalPath(originalPath)
	op.setNewname(newName)
	return op

# Unit test for copy operation.
class CopyOperationUnitTest(unittest.TestCase):
	def setUp(self):
		if os.path.exists('copytodir'):
			shutil.rmtree('copytodir')
		
	def tearDown(self):
		if os.path.exists('copytodir'):
			shutil.rmtree('copytodir')
		
	def test_copy(self):
		import controller
		op = CopyOperation()
		context = controller.BuildController()
		context.setBaseDirectory('.')
		op.setContext(context)
		op.setToPath('copytodir')
		op.addFromPath(os.path.join('vectors', 'afile.c'))
		op.addFromPath(os.path.join('vectors', 'bfile.c'))
		op.addFromPath('vectors')
		op.run()
		
		self.check_copy_results()
		
	def check_copy_results(self):
		self.assert_(os.path.exists('copytodir'))
		self.assert_(os.path.isdir('copytodir'))
		
		self.assert_(os.path.exists(os.path.join('copytodir', 'afile.c')))
		self.assert_(os.path.isfile(os.path.join('copytodir', 'afile.c')))
		
		self.assert_(os.path.exists(os.path.join('copytodir', 'bfile.c')))
		self.assert_(os.path.isfile(os.path.join('copytodir', 'bfile.c')))
		
		self.assert_(os.path.exists(os.path.join('copytodir', 'vectors')))
		self.assert_(os.path.isdir(os.path.join('copytodir', 'vectors')))
		
		self.assert_(os.path.exists(os.path.join('copytodir', 'vectors', 'afile.c')))
		self.assert_(os.path.isfile(os.path.join('copytodir', 'vectors', 'afile.c')))
		
		self.assert_(os.path.exists(os.path.join('copytodir', 'vectors', 'bfile.c')))
		self.assert_(os.path.isfile(os.path.join('copytodir', 'vectors', 'bfile.c')))
	
	def test_copy_fn(self):
		controller.buildContext = controller.BuildController()
		controller.buildContext.setBaseDirectory('.')
		
		copy('copytodir', [os.path.join('vectors', 'afile.c'), os.path.join('vectors', 'bfile.c'), 'vectors']).run()
		self.check_copy_results()

# Unit test for move operation.
class MoveOperationUnitTest(unittest.TestCase):
	def test_move(self):
		pass
	
	def test_move_fn(self):
		pass

# Unit test for delete operation.
class DeleteOperationUnitTest(unittest.TestCase):
	def test_delete(self):
		pass

# Return a suite containing all the test cases for this module.
def suite():
	copySuite = unittest.makeSuite(CopyOperationUnitTest)
	moveSuite = unittest.makeSuite(MoveOperationUnitTest)
	deleteSuite = unittest.makeSuite(DeleteOperationUnitTest)
	suite = unittest.TestSuite((copySuite, moveSuite, deleteSuite))
	return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
