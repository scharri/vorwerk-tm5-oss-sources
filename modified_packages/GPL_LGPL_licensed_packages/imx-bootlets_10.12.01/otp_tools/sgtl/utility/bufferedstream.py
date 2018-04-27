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

import unittest

__all__ = ["BufferedCharacterStream", "BufferedStringCharacterStream", "suite"]

# This class doesn't really do much that the standard file object
# couldn't do itself when in buffered mode, but it provides a nice base for
# adding additional functionality if required. The API is also cleaner.
class BufferedCharacterStream:
	
	CHUNK_SIZE = 128

	def __init__(self, inputFile):
		"""Accepts a single input file object."""
		self.inputFile = inputFile
		self.reset()
	
	def reset(self):
		"""Back up to the start of the stream."""
		self.inputBuffer = ""
		self.charsRead = 0
		
		# get file length
		self.inputFile.seek(0, 2)   # move to end of file
		self.inputLength = self.inputFile.tell()
		self.inputFile.seek(0, 0)	# move to head of file
	
	def readChunk(self):
		"""Reads the next chunk from the file."""
		return self.inputFile.read(self.CHUNK_SIZE)

	def get(self, count=1):
		"""Returns the next count characters as a string from the input file. If
		EOF is hit, as many characters as could be read are returned. If none could
		be read, then None is returned."""

		if self.remainingCharacters() == 0:
			return None

		# read chunk from file
		if len(self.inputBuffer) < count:
			newChunk = self.readChunk()
			self.inputBuffer += newChunk
			if len(self.inputBuffer) == 0:
				return None # hit EOF
			self.charsRead += len(newChunk)

		# return next chars from input buffer
		result = self.inputBuffer[0:count]
		self.inputBuffer = self.inputBuffer[count:]
		return result

	def put(self, s):
		"""Puts the string s at the head of the stream, so that the first
		character of it will be returned by the next call to get()."""
		if s != None:
			self.inputBuffer = s + self.inputBuffer

	def peek(self, count=1):
		"""Has the same effect as calling get() followed by put()."""
		c = self.get(count)
		if c != None:
			self.put(c)
		return c

	def remainingCharacters(self):
		"""Returns the number of characters remaining to be read."""
		return self.inputLength - self.charsRead + len(self.inputBuffer)

# Subclass of BufferedCharacterStream, but operates on strings instead of files.
class BufferedStringCharacterStream(BufferedCharacterStream):
	def __init__(self, inputString):
		self.inputString = inputString
		self.reset()
	
	def reset(self):
		"""We basically pretend that we've already buffered the whole file in memory."""
		self.inputBuffer = self.inputString
		self.inputLength = len(self.inputString)
		self.charsRead = self.inputLength
	
	def readChunk(self):
		""" Return an empty string since all characters are already in memory."""
		return ""

def suite():
	# Tests common to both file and string stream unit tests.
	class BaseBufferedStreamUnitTest(unittest.TestCase):
		def test_get_put(self):
			self.assertEqual(self.stream.remainingCharacters(), 19)
			
			self.assertEqual(self.stream.get(), 't')
			self.assertEqual(self.stream.remainingCharacters(), 18)
			
			self.assertEqual(self.stream.get(), 'h')
			self.assertEqual(self.stream.remainingCharacters(), 17)
			
			self.assertEqual(self.stream.get(), 'e')
			self.assertEqual(self.stream.remainingCharacters(), 16)
			
			self.stream.put('e')
			self.assertEqual(self.stream.remainingCharacters(), 17)
		
		def test_multiple_get_put(self):
			self.assertEqual(self.stream.remainingCharacters(), 19)
			
			self.assertEqual(self.stream.get(4), "the ")
			self.assertEqual(self.stream.remainingCharacters(), 15)
			
			self.stream.put("e ")
			self.assertEqual(self.stream.remainingCharacters(), 17)
			
			self.assertEqual(self.stream.get(5), "e qui")
			self.assertEqual(self.stream.remainingCharacters(), 12)
		
		def test_reset(self):
			self.test_get_put()
			self.stream.reset()
			self.assertEqual(self.stream.remainingCharacters(), 19)
			self.assertEqual(self.stream.get(), 't')
			self.assertEqual(self.stream.remainingCharacters(), 18)
		
		def test_peek(self):
			self.assertEqual(self.stream.remainingCharacters(), 19)
			
			self.assertEqual(self.stream.peek(), "t")
			self.assertEqual(self.stream.remainingCharacters(), 19)
			
			self.assertEqual(self.stream.get(), 't')
			self.assertEqual(self.stream.remainingCharacters(), 18)
			
			self.assertEqual(self.stream.peek(4), "he q")
			self.assertEqual(self.stream.remainingCharacters(), 18)
			
			self.assertEqual(self.stream.get(), 'h')
			self.assertEqual(self.stream.remainingCharacters(), 17)
	
	# Unit test for file streams.
	class BufferedStreamUnitTest(BaseBufferedStreamUnitTest):
		def setUp(self):
			self.file = file("__buffered_stream_test.txt", "w+")
			self.file.write("the quick brown fox")
			self.file.close()
			
			self.file = file("__buffered_stream_test.txt", "r")
			self.stream = BufferedCharacterStream(self.file)
		
		def tearDown(self):
			import os
			self.file.close()
			self.file = None
			os.unlink("__buffered_stream_test.txt")
			
	# Unit test for string streams.
	class BufferedStringStreamUnitTest(BaseBufferedStreamUnitTest):
		def setUp(self):
			self.stream = BufferedStringCharacterStream("the quick brown fox")
	
	streamSuite = unittest.makeSuite(BufferedStreamUnitTest)
	stringSuite = unittest.makeSuite(BufferedStringStreamUnitTest)
	suite = unittest.TestSuite()
	suite.addTests((streamSuite, stringSuite))
	return suite

# Run unit test when called directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
