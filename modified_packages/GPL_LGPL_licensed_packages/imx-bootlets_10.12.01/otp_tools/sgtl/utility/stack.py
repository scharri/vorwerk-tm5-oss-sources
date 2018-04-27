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

import types
import unittest

__all__ = ["Stack", "suite"]

class Stack:
	def __init__(self):
		self.stack = []
	
	def push(self, value):
		"""Push a single value onto the stack."""
		self.stack.append(value)
	
	def pushMultiple(self, values):
		"""Push multiple values onto the stack. The values argument must be a sequence.
		Items are pushed in left to right, or increasing index, order."""
		for i in values:
			self.stack.append(i)
	
	def pop(self):
		return self.stack.pop()
	
	def isEmpty(self):
		return len(self.stack) == 0
	
	def itemCount(self):
		return len(self.stack)
	
	def peek(self, itemIndex=0):
		return self.stack[-itemIndex - 1]
	
	def __len__(self):
		return len(self.stack)
	
	def __getitem__(self, key):
		"""Same as peek(key)."""
		if type(key) == types.IntType:
			return self.stack[-key - 1]
		else:
			raise Exception("Unsupported key type")
	
	def __str__(self):
		return str(self.stack)

class StackUnitTest(unittest.TestCase):
	def setUp(self):
		self.stack = Stack()
	
	def tearDown(self):
		pass
	
	def test_basics(self):
		self.assert_(self.stack.isEmpty())
		self.stack.push(1)
		self.assert_(not self.stack.isEmpty())
		self.stack.pop()
		self.assert_(self.stack.isEmpty())
	
	def test_len(self):
		# test 0 len
		self.assertEqual(len(self.stack), self.stack.itemCount())
		self.assertEqual(len(self.stack), 0)
		
		# test non-zero len
		self.stack.push(1)
		self.stack.push(2)
		self.stack.push(3)
		self.assertEqual(len(self.stack), self.stack.itemCount())
		self.assertEqual(len(self.stack), 3)
	
	def test_push_multiple(self):
		self.stack.pushMultiple([1, 2, 3, 4])
		self.assertEqual(len(self.stack), 4)
		self.assertEqual(self.stack.pop(), 4)
		self.assertEqual(self.stack.pop(), 3)
		self.assertEqual(self.stack.pop(), 2)
		self.assertEqual(self.stack.pop(), 1)
	
	def test_peek(self):
		self.stack.pushMultiple([1, 3, 4])
		self.assertEqual(self.stack.peek(), 4)
		self.stack.pop()
		self.assertEqual(self.stack.peek(), 3)
		self.stack.pop()
		self.assertEqual(self.stack.peek(), 1)
		self.stack.push(10)
		self.assertEqual(self.stack.peek(), 10)

def suite():
	suite = unittest.makeSuite(StackUnitTest)
	return suite

if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
