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
import string
import unittest

# this is a little hack to let python find a sibling package
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)

from utility import bufferedstream
from utility import stack
from utility import lexerbase

# Symbols to be imported when using 'import *'.
__all__ = ['ECLexer', 'ECParserDelegate', 'ECParser', 'suite']

# Subclasses for specific lexical tokens.
class ECTextToken(lexerbase.TokenBase): pass
class ECOpenElementToken(lexerbase.TokenBase): pass
class ECCloseElementToken(lexerbase.TokenBase): pass
class ECIdentifierToken(lexerbase.TokenBase): pass
class ECStringToken(lexerbase.TokenBase): pass
class ECEqualsToken(lexerbase.TokenBase): pass
class ECSlashToken(lexerbase.TokenBase): pass
class ECOpenPropertyToken(lexerbase.TokenBase): pass
class ECClosePropertyToken(lexerbase.TokenBase): pass

class ECParseException(Exception):
	"""Class of exception raised by both the ECLexer and ECParser classes."""
	
	def __init__(self, msg, line):
		self.msg = msg
		self.line = line
		
	def __str__(self):
		return "%s (line %d)" % (self.msg, self.line)

class ECLexer(lexerbase.LexerBase):
	"""This is the lexical analyzer class. It takes a BufferedCharacterStream instance
	from which it will read. The nextToken() method is called by the parser every
	time a new token is needed.
	
	XXX tag tokens with line number (and file when we have includes)
	XXX support inclusion stack
	XXX add property marker support
	"""
	
	# Lexer states
	START_STATE = 1
	ENTER_ELEMENT_STATE = 2
	ELEMENT_STATE = 3
	XML_COMMENT_STATE = 4
	STRING_STATE = 5
	EOF_STATE = 6
	
	def __init__(self, stream):
		lexerbase.LexerBase.__init__(self, stream)
		self._state = ECLexer.START_STATE
	
	def setState(self, newState):
		self._state = newState
	
	def readNextToken(self):
		"""Reads characters from the input stream until a complete token is
		discovered, in which case the new token is returned."""
		
		if self._state == ECLexer.EOF_STATE:
			return None
		
		buffer = ""
		
		while True:
			# Get next char from stream. exit while if we hit EOF.
			c = self.get()
			if c == None:
				break
			
			if self._state == ECLexer.START_STATE:
				if c == '<':
					# This char can either mean we're starting an XML-style comment
					# or starting an XML element. In either case we need to return
					# the text token.
					if self.peek(3) == '!--':
						self._state = ECLexer.XML_COMMENT_STATE
					else:
						self._state = ECLexer.ENTER_ELEMENT_STATE
						self.put(c)
					
					# Remove the last EOL before an XML comment start (if any)
#					if self._state == ECLexer.XML_COMMENT_STATE and len(buffer):
#						buffer = self.removeTrailingLine(buffer)
					
					# only return a text token if the buffer is not empty
					if len(buffer) > 0:
						return ECTextToken(buffer)
				elif c in lexerbase.CRLF:
					buffer += self.scanEOL(c)
				else:
					# Save this char for later
					buffer += c
			elif self._state == ECLexer.ENTER_ELEMENT_STATE:
				# This state exists only to return the open element token.
				# We immediately switch to the real in-element state.
				if c != '<':
					raise ECParseException("unexpected character in ENTER_ELEMENT_STATE", self.getLineNumber())
				self._state = ECLexer.ELEMENT_STATE
				return ECOpenElementToken()
			elif self._state == ECLexer.ELEMENT_STATE:
				if c in lexerbase.lettersAndUnderscore:
					return ECIdentifierToken(c + self.readIdentifier())
				elif c == '=':
					return ECEqualsToken()
				elif c == '"':
					self._state = ECLexer.STRING_STATE
				elif c == '/':
					return ECSlashToken()
				elif c == '>':
					# Close of this element. Switch back to the start state and 
					# return a close element token.
					self._state = ECLexer.START_STATE
					return ECCloseElementToken()
				elif c in lexerbase.whitespace:
					if c in lexerbase.CRLF:
						self.scanEOL(c)
				else:
					raise ECParseException("unexpected character encountered: " + c, self.getLineNumber())
			elif self._state == ECLexer.XML_COMMENT_STATE:
				# Ignore all characters except for the close comment.
				if c == '-' and self.peek(2) == '->':
					# End of this comment, switch back to the start state after
					# skipping the close comment chars.
					self.get(2)
					self._state = ECLexer.START_STATE
					
					# If there is only whitespace up to the EOL immediately after the close
					# comment, remove it and the EOL.
					self.skipWhitespaceToEOL()
				elif c in lexerbase.CRLF:
					# Increment line count within XML comments
					self.scanEOL(c)
			elif self._state == ECLexer.STRING_STATE:
				if c == '\\':
					# Escaped character within the string.
					# XXX support standard C escape chars.
					c = self.get()
					if c is None:
					    raise ECParseException("unexpected end of string", self.getLineNumber())
					buffer += c
				elif c == '"':
					# End of string. Switch back to element state and return token.
					# We always return a string token, even if the buffer is empty, unlike
					# for text tokens.
					self._state = ECLexer.ELEMENT_STATE
					return ECStringToken(buffer)
				elif c in lexerbase.CRLF:
					buffer += self.scanEOL(c)
				else:
					buffer += c
			else:
				raise ECParseException("invalid lexer state", self.getLineNumber())
		
		# We only reach here when EOF is hit. Send a final token, if any.
		lastState = self._state
		self._state = ECLexer.EOF_STATE
		if lastState == ECLexer.START_STATE and len(buffer) > 0:
			return ECTextToken(buffer)
		elif lastState in [ECLexer.ELEMENT_STATE, ECLexer.XML_COMMENT_STATE, ECLexer.STRING_STATE]:
			raise ECParseException("unexpected end of file", self.getLineNumber())
		
		return None
	
	def skipWhitespaceToEOL(self):
		whitespaceLookahead = ""
		while True:
			c = self.get()
			whitespaceLookahead += c
			if not c in lexerbase.whitespace:
				# Non-whitespace before the EOL, so put everything back.
				self.put(whitespaceLookahead)
				break
			elif c in lexerbase.CRLF:
				self.scanEOL(c)
				break
	
	def removeTrailingLine(self, s):
		minusLength = -len(s)
		i = -1
		while i >= minusLength:
			c = s[i]
			if not c in lexerbase.whitespace:
				# We hit a non-whitespace char before we hit an EOL, so just return
				# the original string.
				return s
			elif c in lexerbase.CRLF:
				# Look for CRLF pair
				if c == lexerbase.LF and (i-1 >= minusLength) and s[i-1] == lexerbase.CR:
					i -= 1
				break
			
			i -= 1
		
		return s[:i]

class ECParserDelegate:
	"""Abstract base class for delegates of ECParser."""
	
	def handleContent(self, data): pass
	def handleOpenElement(self, name, attributes): pass
	def handleCloseElement(self, name): pass

class ECParser:
	"""This is the parser class. It takes tokens produced by the ECLexer instance
	and forms them into higher level constructions with more semantic meaning.
	It uses the recursive descent style to recognize entities. 
	
	A delegate (subclass of ECParserDelegate) is responsible for actually doing something with
	what the parser sees. Basically, this class just acts as a simple SAX type XML parser."""
	
	def __init__(self, lexer):
		self._lexer = lexer
		self._delegate = None
	
	def setDelegate(self, delegate):
		self._delegate = delegate
	
	def parse(self):
		while True:
			token = self._lexer.nextToken()
			if not token:
				break
			
			if isinstance(token, ECTextToken):
				if self._delegate:
					self._delegate.handleContent(token.getValue())
			elif isinstance(token, ECOpenElementToken):
				self.parseElement()
	
	def parseElement(self):
		closed = False
		
		# either IDENT or / determines if open or close element
		token = self._lexer.nextToken()
		if isinstance(token, ECSlashToken):
			self.parseCloseElement()
			return
		elif not isinstance(token, ECIdentifierToken):
			raise ECParseException("invalid element", self._lexer.getLineNumber())
		
		tokenName = token.getValue()
		attributes = self.parseAttributes()
		
		# either /> or > must follow the attributes.
		token = self._lexer.nextToken()
		if isinstance(token, ECSlashToken):
			closed = True
			
			# a > token must follow
			token = self._lexer.nextToken()
			if not isinstance(token, ECCloseElementToken):
				raise ECParseException("invalid element", self._lexer.getLineNumber())
		elif not isinstance(token, ECCloseElementToken):
			raise ECParseException("invalid element", self._lexer.getLineNumber())
		
		# let delegate handle open element
		if self._delegate:
			self._delegate.handleOpenElement(tokenName, attributes)
		
		# let delegate handle close element for singleton
		if closed and self._delegate:
			self._delegate.handleCloseElement(tokenName)
	
	def parseAttributes(self):
		attributes = {}
		
		while True:
			# look for attribute name. if another token is encounterd, push it
			# back to lexer and bail.
			token = self._lexer.nextToken()
			if not isinstance(token, ECIdentifierToken):
				self._lexer.pushToken(token)
				return attributes
			
			name = token.getValue()
			
			# look for equals sign
			token = self._lexer.nextToken()
			if not isinstance(token, ECEqualsToken):
				raise ECParseException("unexpected '='", self._lexer.getLineNumber())
			
			# look for attribute value
			token = self._lexer.nextToken()
			if not isinstance(token, ECStringToken):
				raise ECParseException("attribute is missing value", self._lexer.getLineNumber())
			
			# add new attribute
			attributes[name] = token.getValue()
	
	def parseCloseElement(self):
		token = self._lexer.nextToken()
		if not isinstance(token, ECIdentifierToken):
			raise ECParseException("invalid close element", self._lexer.getLineNumber())
		elementName = token.getValue()
		
		# a > must follow
		token = self._lexer.nextToken()
		if not isinstance(token, ECCloseElementToken):
			raise ECParseException("invalid close element", self._lexer.getLineNumber())
		
		# let delegate handle
		if self._delegate:
			self._delegate.handleCloseElement(elementName)


# Returns a suite of all test cases in this source file.
def suite():
	# Unit test for ECLexer class.
	class ECLexerUnitTest(unittest.TestCase):
		# Returns the list of tuples of tokens and corresponding line numbers that 
		# is expected (*after* the token) from the test vector file "vectors/lexer_test.tgpj".
		def getLexerTestTokenVector(self):
			return [	( ECTextToken("#!gbuild\n"), 2 ),
						# skipped a comment here
						#( ECTextToken("\n"), 3 ), # this EOL was removed since it was on the same line as the XML comment
						# <element>
						( ECOpenElementToken(), 3 ),
						( ECIdentifierToken("element"), 3 ),
						( ECCloseElementToken(), 3 ),
						# \n
						( ECTextToken("\n"), 4 ),
						# </element>
						( ECOpenElementToken(), 4 ),
						( ECSlashToken(), 4 ),
						( ECIdentifierToken("element"), 4 ),
						( ECCloseElementToken(), 4 ),
						# \n
						( ECTextToken("\n"), 5 ),
						# <singleton/>
						( ECOpenElementToken(), 5 ),
						( ECIdentifierToken("singleton"), 5 ),
						( ECSlashToken(), 5 ),
						( ECCloseElementToken(), 5 ),
						# \n
						( ECTextToken("\n"), 6 ),
						# <project name="example">
						( ECOpenElementToken(), 6 ),
						( ECIdentifierToken("project"), 6 ),
						( ECIdentifierToken("name"), 6 ),
						( ECEqualsToken(), 6 ),
						( ECStringToken("example"), 6 ),
						( ECCloseElementToken(), 6 ),
						# <description>
						( ECOpenElementToken(), 6 ),
						( ECIdentifierToken("description"), 6 ),
						( ECCloseElementToken(), 6 ),
						# \nSample project with embedded configurations.\n
						( ECTextToken("\nSample project with embedded configurations.\n"), 8 ),
						# </description>
						( ECOpenElementToken(), 8 ),
						( ECSlashToken(), 8 ),
						( ECIdentifierToken("description"), 8 ),
						( ECCloseElementToken(), 8 ),
						# </project>
						( ECOpenElementToken(), 8 ),
						( ECSlashToken(), 8 ),
						( ECIdentifierToken("project"), 8 ),
						( ECCloseElementToken(), 8 ),
						# \nmacro FOO=bar\n
						( ECTextToken("\nmacro FOO=bar\n"), 10 ),
						# <config name="test \"" attr="yes"/>
						( ECOpenElementToken(), 10 ),
						( ECIdentifierToken("config"), 10 ),
						( ECIdentifierToken("name"), 10 ),
						( ECEqualsToken(), 10 ),
						( ECStringToken('test "'), 10 ),
						( ECIdentifierToken("attr"), 10 ),
						( ECEqualsToken(), 10 ),
						( ECStringToken("yes"), 10 ),
						( ECSlashToken(), 10 ),
						( ECCloseElementToken(), 10 ),
						# \n[Project]\nfile.c
						( ECTextToken("\n[Project]\nfile.c\n"), 13 ) ]
		
		# This test case compares the lexer output for vectors/lexer_test.tgpj with
		# the expected set of tokens. Line numbers are checked too.
		def test_lexer_output(self):
			testpath = os.path.join('vectors', 'lexer_test.tgpj')
			testfile = file(testpath, "r")
			stream = bufferedstream.BufferedCharacterStream(testfile)
			lexer = ECLexer(stream)
			
			testVectorTokens = self.getLexerTestTokenVector()
			
			i = 0
			while True:
				token = lexer.nextToken()
				if token == None:
					break
				
				self.assertTrue(i < len(testVectorTokens))
				vectorToken, vectorLine = testVectorTokens[i]
				actualLine = lexer.getLineNumber()
				
				#print "lines:", actualLine, vectorLine, "classes:", token, vectorToken, "values:", token.getValue(), vectorToken.getValue()
				self.assertTrue(isinstance(token, vectorToken.__class__))
				self.assertEqual(token.getValue(), vectorToken.getValue())
				self.assertEqual(actualLine, vectorLine)
				
				i += 1
		
		# This test sends erroneous input through the lexer and checks for exceptions.
		# XXX implement me
		def test_lexer_error(self):
			pass
		
		# Test the remove trailing line method.
		def test_remove_trailing_line(self):
			lexer = ECLexer(None)
			
			line = "hello\n"
			removed = lexer.removeTrailingLine(line)
			self.assertEqual(removed, "hello")
			
			line = "hello"
			removed = lexer.removeTrailingLine(line)
			self.assertEqual(removed, "hello")
			
			line = "hello\n   \t"
			removed = lexer.removeTrailingLine(line)
			self.assertEqual(removed, "hello")
			
			line = "hello\n\n"
			removed = lexer.removeTrailingLine(line)
			self.assertEqual(removed, "hello\n")
			
			line = "hello\r\n"
			removed = lexer.removeTrailingLine(line)
			self.assertEqual(removed, "hello")
			
			line = "hello\r"
			removed = lexer.removeTrailingLine(line)
			self.assertEqual(removed, "hello")
			
			line = "hello\n\nyo   "
			removed = lexer.removeTrailingLine(line)
			self.assertEqual(removed, "hello\n\nyo   ")
	
	# Unit test for ECParser class.
	class ECParserUnitTest(unittest.TestCase):
		
		# Callback types
		CONTENT = 1
		OPEN_ELEM = 2
		CLOSE_ELEM = 3
		
		def setUp(self):
			self.calls = []
		
		def tearDown(self):
			pass
		
		def handleContent(self, data):
			#print "content:", data
			self.calls.append((ECParserUnitTest.CONTENT, data))
		
		def handleOpenElement(self, name, attributes):
			#print "open:", name, attributes
			self.calls.append((ECParserUnitTest.OPEN_ELEM, name, attributes))
		
		def handleCloseElement(self, name):
			#print "close:", name
			self.calls.append((ECParserUnitTest.CLOSE_ELEM, name))
		
		# This method creates the stream, lexer, and parser necessary to parse the
		# data passed in via the 'text' argument. The parse() method is called on
		# the parser automatically. Nothing is returned. The handleX methods take
		# care of that.
		#
		# The self.calls list is cleared before the data is parsed. This allows multiple
		# calls to this method within one test case.
		def parseText(self, text):
			self.calls = []
			stream = bufferedstream.BufferedStringCharacterStream(text)
			parser = ECParser(ECLexer(stream))
			parser.setDelegate(self)
			parser.parse()
	
		# Parse a full template file and compare the set of callbacks with those that
		# we expect to see.
		def test_parse_file(self):
			testpath = os.path.join('vectors', 'lexer_test.tgpj')
			testfile = file(testpath, "r")
			stream = bufferedstream.BufferedCharacterStream(testfile)
			parser = ECParser(ECLexer(stream))
			parser.setDelegate(self)
			
			parser.parse()
			
			expectedCalls = [(1, '#!gbuild\n'), (2, 'element', {}), (1, '\n'), (3, 'element'), (1, '\n'),
				(2, 'singleton', {}), (3, 'singleton'), (1, '\n'), (2, 'project', {'name': 'example'}),
				(2, 'description', {}), (1, '\nSample project with embedded configurations.\n'),
				(3, 'description'), (3, 'project'), (1, '\nmacro FOO=bar\n'), (2, 'config', {'name': 'test "', 'attr': 'yes'}),
				(3, 'config'), (1, '\n[Project]\nfile.c\n')]
			
			self.assertEqual(self.calls, expectedCalls)
			#print "calls:", self.calls
		
		# Test simple input.
		def test_singleton_element(self):
			self.parseText("<singleton/>")
			self.assertEqual(self.calls, [(2, 'singleton', {}), (3, 'singleton')])
			
			self.parseText("<singleton attr1=\"foo\" attr2=\"bar\" attr3=\"12345 78935\"/>")
			self.assertEqual(self.calls, [(2, 'singleton', {'attr1':'foo', 'attr2':'bar', 'attr3':'12345 78935'}),
							  (3, 'singleton')])
		
		# Test elements with text content.
		def test_element_content(self):
			self.parseText("<elem attr=\"yes\">this is some content</elem>")
			self.assertEqual(self.calls, [(2, 'elem', {'attr':'yes'}), (1, 'this is some content'),
							  (3, 'elem')])
			
		# Test elements within elements.
		def test_container_elements(self):
			self.parseText('<parent><child/></parent>')
			self.assertEqual(self.calls, [(2, 'parent', {}), (2, 'child', {}), (3, 'child'), (3, 'parent')])
			
	lexerSuite = unittest.makeSuite(ECLexerUnitTest)
	parserSuite = unittest.makeSuite(ECParserUnitTest)
	suite = unittest.TestSuite()
	suite.addTests((lexerSuite, parserSuite))
	return suite

# Run unit tests when this source file is executed directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())

