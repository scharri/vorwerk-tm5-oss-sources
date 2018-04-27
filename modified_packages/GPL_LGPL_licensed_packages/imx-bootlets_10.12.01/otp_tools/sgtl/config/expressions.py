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

# this is a little hack to let python find a sibling package
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)

from utility import bufferedstream
from utility import stack
from utility.lexerbase import *

# Operator string constants.
AND_OP_STR = '&&'
OR_OP_STR = '||'
XOR_OP_STR = '^^'
NOT_OP_STR = '!'

# Binary operators.
AND_OP = 1
OR_OP = 2
XOR_OP = 3

class ECExpressionDelegate:
	def getVariableValue(self, identifier):
		pass

class ECExpression:
	def evaluate(self, delegate):
		return False

class ECExpressionBinaryOp(ECExpression):
	def __init__(self, left, op, right):
		self._left = left
		self._right = right
		self._op = op
		
	def evaluate(self, delegate):
		assert self._left != None
		assert self._right != None
		
		leftValue = self._left.evaluate(delegate)
		rightValue = self._right.evaluate(delegate)
		result = False
		
		if self._op == AND_OP:
			result = leftValue & rightValue
		elif self._op == OR_OP:
			result = leftValue | rightValue
		elif self._op == XOR_OP:
			result = leftValue ^ rightValue
		
		return result
		
	def __eq__(self, other):
		return (self.__class__ == other.__class__ and self._left == other._left and self._op == other._op and self._right == other._right)
	
	def __str__(self):
		ops = ['?', AND_OP_STR, OR_OP_STR, XOR_OP_STR]
		return "<%s:0x%x (%s) %s (%s)>" % (str(self.__class__), id(self), str(self._left), ops[self._op], str(self._right))

class ECExpressionNot(ECExpression):
	def __init__(self, expr):
		self._expr = expr
		
	def evaluate(self, delegate):
		assert self._expr != None
		
		return not self._expr.evaluate(delegate)
		
	def __eq__(self, other):
		return (self.__class__ == other.__class__ and self._expr == other._expr)
	
	def __str__(self):
		return "<%s:0x%x !(%s)>" % (str(self.__class__), id(self), str(self._expr))

class ECExpressionConstant(ECExpression):
	def __init__(self, value):
		self._value = value
	
	def evaluate(self, delegate):
		return self._value
	
	def __eq__(self, other):
		return (self.__class__ == other.__class__ and self._value == other._value)
	
	def __str__(self):
		return "<%s:0x%x %s>" % (str(self.__class__), id(self), str(self._value))

class ECExpressionVariable(ECExpression):
	def __init__(self, ident):
		self._ident = ident
		
	def evaluate(self, delegate):
		assert self._ident != None
		
		if delegate:
			return delegate.getVariableValue(self._ident)
		else:
			return False
	
	def __eq__(self, other):
		return (self.__class__ == other.__class__ and self._ident == other._ident)
	
	def __str__(self):
		return "<%s:0x%x %s>" % (str(self.__class__), id(self), self._ident)

# Tokens produced by the boolean expression lexical analyser.
class ECExpressionIdentifierToken(TokenBase): pass
class ECExpressionOperatorToken(TokenBase): pass
class ECExpressionOpenParenToken(TokenBase): pass
class ECExpressionCloseParenToken(TokenBase): pass
class ECExpressionConstantToken(TokenBase): pass

# Class of exception raised by the boolean expression lexer and parser.
class ECExpressionParseException(Exception): pass

class ECExpressionLexer(LexerBase):
	"""Lexical analyser for boolean expressions."""
	
	def readNextToken(self):
		while True:
			c = self.get()
			if not c:
				break
			
			if c in lettersAndUnderscore:
				ident = c + self.readIdentifier()
				if ident in ["true", "false"]:
					return ECExpressionConstantToken(ident)
				else:
					return ECExpressionIdentifierToken(ident)
			elif c in "&|^":
				op = c + self.get()
				if op in [AND_OP_STR, OR_OP_STR, XOR_OP_STR]:
					return ECExpressionOperatorToken(op)
				else:
					raise ECExpressionParseException("Unexpected character '%s'" % c)
			elif c == '!':
				return ECExpressionOperatorToken(NOT_OP_STR)
			elif c == '(':
				return ECExpressionOpenParenToken()
			elif c == ')':
				return ECExpressionCloseParenToken()
			elif c in whitespace:
				pass
			else:
				raise ECExpressionParseException("Unexpected character '%s'" % c)
		
		return None

class ECExpressionParser:
	"""Parser for boolean expressions.
	
	The expression grammar is as follows:
	
		expr			::=		and-expr
						|		expr '||' and-expr
		
		and-expr		::=		xor-expr
						|		and-expr '&&' xor-expr
		
		xor-expr		::=		unary-expr
						|		xor-expr '^^' unary-expr
		
		unary-expr		::=		primary-expr
						|		'!' unary-expr
		
		primary-expr	::=		constant
						|		IDENT
						|		paren-expr
		
		paren-expr		::=		'(' expr ')'
		
		constant		::=		'true'
						|		'false'
	
	"""
	
	def __init__(self, expression):
		self._expression = expression
		self._stream = bufferedstream.BufferedStringCharacterStream(expression)
		self._lexer = ECExpressionLexer(self._stream)		
		
	def parseParenExpr(self):
		token = self._lexer.nextToken()
		if not token:
			return None
		
		if not isinstance(token, ECExpressionOpenParenToken):
			raise ECExpressionParseException("unexpected token: " + str(token))
		
		expr = self.parseExpr()
		if not expr:
			raise ECExpressionParseException("no expression between parentheses")
		
		token = self._lexer.nextToken()
		if not (token and isinstance(token, ECExpressionCloseParenToken)):
			raise ECExpressionParseException("expected ')'")
		
		return expr
	
	def parsePrimaryExpr(self):
		token = self._lexer.nextToken()
		if not token:
			return None
		
		if isinstance(token, ECExpressionOpenParenToken):
			self._lexer.pushToken(token)
			return self.parseParenExpr()
		elif isinstance(token, ECExpressionConstantToken):
			value = True
			if token.getValue() == "false":
				value = False
			return ECExpressionConstant(value)
		elif isinstance(token, ECExpressionIdentifierToken):
			return ECExpressionVariable(token.getValue())
		else:
			raise ECExpressionParseException("unexpected token: " + str(token))
	
	def parseUnaryExpr(self):
		token = self._lexer.nextToken()
		if not token:
			return None
		
		if not (isinstance(token, ECExpressionOperatorToken) and token.getValue() == "!"):
			self._lexer.pushToken(token)
			return self.parsePrimaryExpr()
		
		expr = self.parseUnaryExpr()
		return ECExpressionNot(expr)
		
	def parseXorExpr(self):
		left = self.parseUnaryExpr()
		if not left:
			return None
		
		token = self._lexer.nextToken()
		if not token:
			return left
		if not (isinstance(token, ECExpressionOperatorToken) and token.getValue() == '^^'):
			self._lexer.pushToken(token)
			return left
		
		right = self.parseXorExpr()
		if not right:
			raise ECExpressionParseException("no righthand expression")
		
		return ECExpressionBinaryOp(left, XOR_OP, right)
		
	def parseAndExpr(self):
		left = self.parseXorExpr()
		if not left:
			return None
		
		token = self._lexer.nextToken()
		if not token:
			return left
		if not (isinstance(token, ECExpressionOperatorToken) and token.getValue() == '&&'):
			self._lexer.pushToken(token)
			return left
		
		right = self.parseAndExpr()
		if not right:
			raise ECExpressionParseException("no righthand expression")
		
		return ECExpressionBinaryOp(left, AND_OP, right)
	
	def parseExpr(self):
		left = self.parseAndExpr()
		if not left:
			return None
		
		token = self._lexer.nextToken()
		if not token:
			return left
		if not (isinstance(token, ECExpressionOperatorToken) and token.getValue() == '||'):
			self._lexer.pushToken(token)
			return left
		
		right = self.parseExpr()
		if not right:
			raise ECExpressionParseException("no righthand expression")
		
		return ECExpressionBinaryOp(left, OR_OP, right)

# Returns a suite of all test cases in this source file.
def suite():
	# Unit tests for expressions.
	class ExpressionLexerUnitTest(unittest.TestCase):
		def checkLexerOutput(self, vector, expectedTokenList):
			stream = bufferedstream.BufferedStringCharacterStream(vector)
			lexer = ECExpressionLexer(stream)
			actualTokenList = []
			while True:
				token = lexer.nextToken()
				if not token:
					break
				actualTokenList.append(token)
				#print token
			
			self.assertEqual(actualTokenList, expectedTokenList)
			
		def test_binary_or(self):
			vector = "a || b"
			expected = [ECExpressionIdentifierToken("a"), ECExpressionOperatorToken(OR_OP_STR), ECExpressionIdentifierToken("b")]
			self.checkLexerOutput(vector, expected)
			
		def test_binary_and(self):
			vector = "a && b"
			expected = [ECExpressionIdentifierToken("a"), ECExpressionOperatorToken(AND_OP_STR), ECExpressionIdentifierToken("b")]
			self.checkLexerOutput(vector, expected)
			
		def test_binary_xor(self):
			vector = "a ^^ b"
			expected = [ECExpressionIdentifierToken("a"), ECExpressionOperatorToken(XOR_OP_STR), ECExpressionIdentifierToken("b")]
			self.checkLexerOutput(vector, expected)
		
		def test_not(self):
			vector = "!foo"
			expected = [ECExpressionOperatorToken(NOT_OP_STR), ECExpressionIdentifierToken("foo")]
			self.checkLexerOutput(vector, expected)
		
		def test_bool_constants(self):
			vector = "true"
			expected = [ECExpressionConstantToken("true")]
			self.checkLexerOutput(vector, expected)
			
			vector = "false"
			expected = [ECExpressionConstantToken("false")]
			self.checkLexerOutput(vector, expected)
		
		def test_parentheses(self):
			vector = "a || (b && c)"
			expected = [ECExpressionIdentifierToken("a"), ECExpressionOperatorToken(OR_OP_STR), ECExpressionOpenParenToken(),
						ECExpressionIdentifierToken("b"), ECExpressionOperatorToken(AND_OP_STR), ECExpressionIdentifierToken("c"),
						ECExpressionCloseParenToken()]
			self.checkLexerOutput(vector, expected)
		
		def test_complex(self):
			vector = "true ^^ !a || (b && c)"
			expected = [ECExpressionConstantToken("true"), ECExpressionOperatorToken(XOR_OP_STR), ECExpressionOperatorToken("!"),
						ECExpressionIdentifierToken("a"), ECExpressionOperatorToken(OR_OP_STR), ECExpressionOpenParenToken(),
						ECExpressionIdentifierToken("b"), ECExpressionOperatorToken(AND_OP_STR), ECExpressionIdentifierToken("c"),
						ECExpressionCloseParenToken()]
			self.checkLexerOutput(vector, expected)
			
		def test_identifiers(self):
			vector = "thisisalongidentifier"
			expected = [ECExpressionIdentifierToken("thisisalongidentifier")]
			self.checkLexerOutput(vector, expected)
			
			vector = "_ident_"
			expected = [ECExpressionIdentifierToken("_ident_")]
			self.checkLexerOutput(vector, expected)
			
			vector = "ident1234567890ident"
			expected = [ECExpressionIdentifierToken("ident1234567890ident")]
			self.checkLexerOutput(vector, expected)
	
	# Unit test for boolean expression parser.
	class ExpressionParserUnitTest(unittest.TestCase):
		def checkParserResults(self, vector, expected):
			parser = ECExpressionParser(vector)
			result = parser.parseExpr()
			#print result
			self.assertEqual(result, expected)
		
		def test_or(self):
			vector = "a || b"
			expected = ECExpressionBinaryOp(ECExpressionVariable('a'), OR_OP, ECExpressionVariable('b'))
			self.checkParserResults(vector, expected)
		
		def test_and(self):
			vector = "a && b"
			expected = ECExpressionBinaryOp(ECExpressionVariable('a'), AND_OP, ECExpressionVariable('b'))
			self.checkParserResults(vector, expected)
		
		def test_xor(self):
			vector = "a ^^ b"
			expected = ECExpressionBinaryOp(ECExpressionVariable('a'), XOR_OP, ECExpressionVariable('b'))
			self.checkParserResults(vector, expected)
		
		def test_ident(self):
			vector = "foo"
			expected = ECExpressionVariable('foo')
			self.checkParserResults(vector, expected)
		
		def test_not(self):
			vector = "! foo"
			expected = ECExpressionNot(ECExpressionVariable('foo'))
			self.checkParserResults(vector, expected)
		
		def test_parentheses(self):
			vector = "(foo || bar) ^^ (baz && raz)"
			expected = ECExpressionBinaryOp(ECExpressionBinaryOp(ECExpressionVariable('foo'), OR_OP, ECExpressionVariable('bar')), XOR_OP,
											 ECExpressionBinaryOp(ECExpressionVariable('baz'), AND_OP, ECExpressionVariable('raz')))
			self.checkParserResults(vector, expected)
		
		def test_constant(self):
			vector = "true && false"
			expected = ECExpressionBinaryOp(ECExpressionConstant(True), AND_OP, ECExpressionConstant(False))
			self.checkParserResults(vector, expected)
			
		def test_empty(self):
			vector = ""
			expected = None
			self.checkParserResults(vector, expected)
			
			vector = "    "
			self.checkParserResults(vector, expected)
		
		def test_complex(self):
			vector = "(nand1 || nand2) && (nand1 ^^ nand2)"
			expected = ECExpressionBinaryOp(ECExpressionBinaryOp(ECExpressionVariable('nand1'), OR_OP, ECExpressionVariable('nand2')),
											 AND_OP, ECExpressionBinaryOp(ECExpressionVariable('nand1'), XOR_OP, ECExpressionVariable('nand2')))
			self.checkParserResults(vector, expected)
		
		def test_close_paren_bug(self):
			vector = "(!mtp && !wm_drm) || (mtp && wm_drm && pd_drm)"
			expected = ECExpressionBinaryOp(ECExpressionBinaryOp(ECExpressionNot(ECExpressionVariable('mtp')), AND_OP, ECExpressionNot(ECExpressionVariable('wm_drm'))),
											 OR_OP, ECExpressionBinaryOp(ECExpressionVariable('mtp'), AND_OP, ECExpressionBinaryOp(ECExpressionVariable('wm_drm'), AND_OP, ECExpressionVariable('pd_drm'))))
			self.checkParserResults(vector, expected)
		
		def test_tri_and_paren(self):
			vector = "(mtp && wm_drm && pd_drm)"
			expected = ECExpressionBinaryOp(ECExpressionVariable('mtp'), AND_OP,
											 ECExpressionBinaryOp(ECExpressionVariable('wm_drm'), AND_OP, ECExpressionVariable('pd_drm')))
			self.checkParserResults(vector, expected)
		
		def test_tri_and(self):
			vector = "mtp && wm_drm && pd_drm"
			expected = ECExpressionBinaryOp(ECExpressionVariable('mtp'), AND_OP,
											 ECExpressionBinaryOp(ECExpressionVariable('wm_drm'), AND_OP, ECExpressionVariable('pd_drm')))
			self.checkParserResults(vector, expected)
	
	# Unit test for expression trees.
	class ExpressionTreeUnitTest(unittest.TestCase, ECExpressionDelegate):
		def setUp(self):
			self._values = {'a':True, 'b':False, 'c':True, 'd':False}
			
		def getVariableValue(self, identifier):
			return self._values[identifier]
		
		def test_constant(self):
			expr = ECExpressionConstant(True)
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionConstant(False)
			self.assertEqual(expr.evaluate(self), False)
			
		def test_not(self):
			expr = ECExpressionNot(ECExpressionConstant(True))
			self.assertEqual(expr.evaluate(self), False)
			
			expr = ECExpressionNot(ECExpressionConstant(False))
			self.assertEqual(expr.evaluate(self), True)
			
		def test_variable(self):
			expr = ECExpressionVariable('a')
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionVariable('b')
			self.assertEqual(expr.evaluate(self), False)
			
		def test_not_variable(self):
			expr = ECExpressionNot(ECExpressionVariable('a'))
			self.assertEqual(expr.evaluate(self), False)
			
			expr = ECExpressionNot(ECExpressionVariable('b'))
			self.assertEqual(expr.evaluate(self), True)
			
		def test_and(self):
			expr = ECExpressionBinaryOp(ECExpressionConstant(True), AND_OP, ECExpressionConstant(True))
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(True), AND_OP, ECExpressionConstant(False))
			self.assertEqual(expr.evaluate(self), False)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(False), AND_OP, ECExpressionConstant(True))
			self.assertEqual(expr.evaluate(self), False)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(False), AND_OP, ECExpressionConstant(False))
			self.assertEqual(expr.evaluate(self), False)
			
		def test_or(self):
			expr = ECExpressionBinaryOp(ECExpressionConstant(True), OR_OP, ECExpressionConstant(True))
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(True), OR_OP, ECExpressionConstant(False))
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(False), OR_OP, ECExpressionConstant(True))
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(False), OR_OP, ECExpressionConstant(False))
			self.assertEqual(expr.evaluate(self), False)
			
		def test_xor(self):
			expr = ECExpressionBinaryOp(ECExpressionConstant(True), XOR_OP, ECExpressionConstant(True))
			self.assertEqual(expr.evaluate(self), False)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(True), XOR_OP, ECExpressionConstant(False))
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(False), XOR_OP, ECExpressionConstant(True))
			self.assertEqual(expr.evaluate(self), True)
			
			expr = ECExpressionBinaryOp(ECExpressionConstant(False), XOR_OP, ECExpressionConstant(False))
			self.assertEqual(expr.evaluate(self), False)
		
		def test_complex(self):
			expr = ECExpressionBinaryOp(ECExpressionNot(ECExpressionVariable('b')), XOR_OP,
										ECExpressionBinaryOp(ECExpressionVariable('c'), AND_OP, ECExpressionVariable('d')))
			self.assertEqual(expr.evaluate(self), True)
	
	lexerSuite = unittest.makeSuite(ExpressionLexerUnitTest)
	parseSuite = unittest.makeSuite(ExpressionParserUnitTest)
	treeSuite = unittest.makeSuite(ExpressionTreeUnitTest)
	suite = unittest.TestSuite()
	suite.addTests((lexerSuite, parseSuite, treeSuite))
	return suite

# Run unit tests when this source file is executed directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
