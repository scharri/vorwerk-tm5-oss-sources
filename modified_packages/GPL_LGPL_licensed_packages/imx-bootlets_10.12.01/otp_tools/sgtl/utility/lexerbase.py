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
import string

import stack

# Exported symbols.
__all__ = ['lettersAndUnderscore', 'identChars', 'CR', 'LF', 'CRLF', 'whitespace', 'TokenBase', 'LexerBase']

# Character set constants
lettersAndUnderscore = string.letters + '_'
identChars = lettersAndUnderscore + string.digits + '-'
CR = '\r'
LF = '\n'
CRLF = CR + LF
whitespace = ' \t' + CRLF

##
# @brief Base class for all lexical tokens.
#
# Token types are identified by the class.
#
# @todo Maybe should add a line number field?
class TokenBase:
	def __init__(self, value=None):
		self._value = value
	
	def getValue(self):
		return self._value
	
	def __eq__(self, other):
		return ((self.__class__ == other.__class__) and (self._value == other._value))
	
	def __str__(self):
		return "<%s:0x%x %s>" % (self.__class__, id(self), str(self._value))

##
# @brief Base class for lexical analysers.
#
class LexerBase:
	def __init__(self, stream):
		self._stream = stream
		self._line = 1
		self._includeStack = stack.Stack()
		self._tokenStack = stack.Stack()
		self._includeFinishedHandler = None
		self._includeFinishedHandlerRefcon = None
	
	def readIdentifier(self):
		buffer = ""
		while True:
			c = self._stream.get()
			if not c:
				return buffer
			elif not c in identChars:
				self._stream.put(c)
				return buffer
			buffer += c
	
	##
	# @brief Set a function to be called when an include file hits EOF.
	def setIncludeFinishedHandler(self, handler, refcon):
		self._includeFinishedHandler = handler
		self._includeFinishedHandlerRefcon = refcon
	
	##
	# @brief Handle including another input file.
	#
	# The current input stream is pushed onto a stack and the input is switched over
	# to @a includeStream. When @a includeStream runs out of data, the input stream
	# is returned to the stream at the top of the include stack.
	#
	# @param self
	# @param includeStream New instance of BufferedCharacterStream to start reading from.
	def pushInclude(self, includeStream):
		self._includeStack.push(self._stream)
		self._stream = includeStream
	
	##
	# @brief Returns the current line number of the input stream.
	def getLineNumber(self):
		return self._line
	
	##
	# @brief Returns token to the top of the token stack.
	# 
	# The returned token will be the next token returned form nextToken().
	def pushToken(self, token):
		self._tokenStack.push(token)
	
	##
	# Either pops a token from the token stack or calls readNextToken()
	# if the stack is empty. 
	def nextToken(self):
		if self._tokenStack.isEmpty():
			return self.readNextToken()
		else:
			return self._tokenStack.pop()
	
	##
	# Returns the next token without removing it from the streak. The same
	# token will be returned from the next call to nextToken().
	def peekToken(self):
		token = self.nextToken()
		self.pushToken(token)
		return token
	
	##
	# @brief Get the next character from the input stream.
	#
	# If the input stream
	def get(self, count=1):
		c = self._stream.get(count)
		while (c is None) and (not self._includeStack.isEmpty()):
			self._stream = self._includeStack.pop()
			if self._includeFinishedHandler is not None:
				self._includeFinishedHandler(self._includeFinishedHandlerRefcon)
			c = self._stream.get(count)
		return c
	
	##
	# @brief Peeks at the next character in the stream.
	def peek(self, count=1):
		c = self.get(count)
		if c != None:
			self.put(c)
		return c
	
	##
	# @brief Returns input to the stream.
	def put(self, s):
		self._stream.put(s)
	
	##
	# @brief Must be overridden by subclass.
	def readNextToken(self):
		return None
	
	##
	# @brief Scans input until an end of line seqauence is reached.
	#
	# This method should be called after encountering a CR or LF (which is passed in c). It
	# increments the line number and skips the LF in a CRLF. The return value is either CR,
	# LF, or CRLF depending on the line ending that was encountered.
	def scanEOL(self, c):
		self._line += 1
		# Skip the LF in CRLF so we don't count it as two lines.
		if c == CR and self._stream.peek(1) == LF:
			return c + self._stream.get(1)
		else:
			return c



		
