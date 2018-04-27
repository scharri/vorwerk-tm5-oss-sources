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
import sys
import unittest
import tree

class ECOperationNode(tree.ECTreeNode):
	def execute(self, context):
		pass

class ECConditionalBlockNode(ECOperationNode):
	def __init__(self, conditional):
		ECOperationNode.__init__(self)
		self._conditional = conditional
	
	def getConditional(self):
		return self._conditional

	def execute(self, context):
		pass
	
	def classSpecificFormat(self):
		return self._conditional
	
	def __eq__(self, other):
		return (tree.ECTreeNode.__eq__(self, other) and (self._conditional == other._conditional))

class ECUnconditionalBlockNode(ECOperationNode):
	def __init__(self, content):
		ECOperationNode.__init__(self)
		self._content = content
	
	def getContent(self):
		return self._content

	def execute(self, context):
		context.write(self._content)
	
	def __eq__(self, other):
		return (tree.ECTreeNode.__eq__(self, other) and (self._content == other._content))
	
# Returns a suite of all test cases in this source file.
def suite():
	# Unit tests.
	class OperationUnitTest(unittest.TestCase):
		def test_operation(self):
			pass
	
	suite = unittest.makeSuite(OperationUnitTest)
	return suite

# Run unit tests when this source file is executed directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
