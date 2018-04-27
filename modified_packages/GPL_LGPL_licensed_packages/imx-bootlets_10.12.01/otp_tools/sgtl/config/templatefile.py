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
import templateparser
import tree
import definitions
import operations

# this is a little hack to let python find a sibling package
parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)

from utility import bufferedstream
from utility import stack

# Symbols to be imported when using 'import *'.
__all__ = ['ECProjectTemplateFile', 'suite']

# Dictionary used to map element names to definition node classes.
definitionElementNames = {
		"project" : definitions.ECProjectDefinition,
		"config" : definitions.ECConfigDefinition,
		"define" : definitions.ECConditionalDefinition,
		"require" : definitions.ECRequirementDefinition,
		"property" : definitions.ECPropertyDefinition
	}

class ECProjectTemplateFile(templateparser.ECParserDelegate):
	"""This class packages the lexer and parser together under a single class.
	Only reading of the template file is supported."""
	
	def __init__(self, templatePath):
		self._path = templatePath
		
		# Create root of the tree.
		self._treeRoot = tree.ECTreeNode()
		self._currentNode = self._treeRoot
		
		self._elementStack = stack.Stack()
		self._currentElement = None
		self._inConditional = 0
		self._inIncludeElement = False
		self._includePath = None
		self._cwdStack = stack.Stack()
	
	def addTreeNode(self, newNode):
		# Add this node as a child of the one we're currently working on.
		self._currentNode.addChild(newNode)
	
	def handleContent(self, data):
		if self._currentElement:
			self._currentElement.handleContent(data)
		else:
			# Unconditional block.
			self.addTreeNode(operations.ECUnconditionalBlockNode(data))
	
	def handleOpenElement(self, name, attributes):
		if self._currentElement:
			# If the current element returns a new element object, it becomes
			# the new current element. The previous one is saved on a stack.
			newElement = self._currentElement.handleOpenElement(name, attributes)
			if newElement:
				self._elementStack.push(self._currentElement)
				self._currentElement = newElement
		elif name in definitionElementNames:
			# create new definition node for this element
			self._currentElement = definitionElementNames[name](name, attributes)
		elif name == 'include':
			# Include element. Note that we saw it and postpone action until the
			# corresponding close element.
			self._inIncludeElement = True
			try:
				self._includePath = attributes['path']
			except KeyError:
				raise templateparser.ECParseException("invalid include element, missing path attribute", self._lexer.getLineNumber())
		else:
			# Must be a conditional block. Save it for later.
			node = operations.ECConditionalBlockNode(name)
			self.addTreeNode(node)
			self._currentNode = node
			self._inConditional += 1
	
	def handleCloseElement(self, name):
		if self._inConditional:
			# Un-"pop" the current node. The new current node becomes the parent
			# of the previous.
			self._currentNode = self._currentNode.getParent()
			self._inConditional -= 1
		elif self._inIncludeElement and name == 'include':
			# This is the close for the include element, so we can handle it now.
			try:
				includeFile = file(self._includePath, "r")
				includeStream = bufferedstream.BufferedCharacterStream(includeFile)
			except:
				print "Could not open include file", self._includePath
				raise
			
			# Save current working directory.
			self._cwdStack.push(os.getcwd())
			
			# Switch working directory to parent of this include file.
			includeDir = os.path.dirname(self._includePath)
			if len(includeDir):
				os.chdir(includeDir)
			
			self._lexer.pushInclude(includeStream)
			
			self._inIncludeElement = False
		elif self._currentElement:
			# If the current element object returns True from handleCloseElement()
			# it means that we're done processing that element.
			if self._currentElement.handleCloseElement(name):
				self.addTreeNode(self._currentElement)
				if self._elementStack.isEmpty():
					self._currentElement = None
				else:
					self._currentElement = self._elementStack.pop()

	def handleIncludeFinished(self):
		# Restore working directory.
		os.chdir(self._cwdStack.pop())
	
	def parseTree(self):
		"""Returns the ECTreeNode that is the root of the tree. Most nodes are siblings
		of this object, not its children.
		
		The file object, stream, lexer, and parser are not saved beyond the life of this
		method, and the template file is not kept open after this method returns."""
		try:
			templateFile = file(self._path, "r")
			stream = bufferedstream.BufferedCharacterStream(templateFile)
		except:
			raise Exception("Could not open template file " + self._path)
		
		# Save current working directory.
		self._cwdStack.push(os.getcwd())
		
		# Switch working directory to parent dir of the template file.
		templateParentPath = os.path.dirname(self._path)
		os.chdir(templateParentPath)
		
		try:
			self._lexer = templateparser.ECLexer(stream)
			self._lexer.setIncludeFinishedHandler(ECProjectTemplateFile.handleIncludeFinished, self)
	
			try:
				theParser = templateparser.ECParser(self._lexer)
				theParser.setDelegate(self)
				theParser.parse()
				templateFile.close()
				return self._treeRoot
			except templateparser.ECParseException, e:
				# just re-raise an ECParseException since it already has the line number
				raise e
			except Exception, e:
				# trap other exceptions and raise an ECParseException with the line number
				# in their place
				raise templateparser.ECParseException(str(e), self._lexer.getLineNumber())
		finally:
			# Restore working directory.
			os.chdir(self._cwdStack.pop())
		
# Returns a suite of all test cases in this source file.
def suite():
	# Unit test for ECProjectTemplateFile class.
	#
	# XXX add negative tests
	class ECProjectTemplateFileUnitTest(unittest.TestCase):
		def prepareVector(self, vectorContents):
			# Construct path and make sure the file doesn't already exist.
			path = os.path.join('vectors', '_test.tgpj')
			self.destroyVector(path)
			
			vector = file(path, "w")
			vector.write(vectorContents)
			vector.close()
			
			return path
			
		def destroyVector(self, vectorPath):
			if os.path.exists(vectorPath):
				os.unlink(vectorPath)
		
		def runVector(self, vectorContents, expectedResult):
			path = self.prepareVector(vectorContents)
			try:
				template = ECProjectTemplateFile(path)
				result = template.parseTree()
#				print result
#				print expectedResult
				self.assertEqual(result, expectedResult)
			finally:
				self.destroyVector(path)
			
		# This test case just makes sure that we can parse a known good project template without errors.
		def test_parse_template(self):
			templatePath = os.path.join('vectors', 'small.tgpj')
			template = ECProjectTemplateFile(templatePath)
			
			result = template.parseTree()
			#print result
		
		# Tests parsing a project definition element.
		def test_project(self):
			vector = """#!gbuild\n<project name="example"><description>hello</description></project>"""
			
			expectedResult = tree.ECTreeNode()
			expectedResult.addChild(operations.ECUnconditionalBlockNode("#!gbuild\n"))
			project = definitions.ECProjectDefinition('project', {'name':'example'})
			project._description = "hello"
			expectedResult.addChild(project)
			
			self.runVector(vector, expectedResult)
		
		# Tests parsing a config definition element.
		def test_config(self):
			vector = """#!gbuild\n<config name="test"/>"""
			
			expectedResult = tree.ECTreeNode()
			expectedResult.addChild(operations.ECUnconditionalBlockNode("#!gbuild\n"))
			expectedResult.addChild(definitions.ECConfigDefinition('config', {'name':'test'}))
			
			self.runVector(vector, expectedResult)
		
		# Tests parsing a config element and corresponding conditional block.
		def test_conditional(self):
			vector = """#!gbuild\n<config name="test"/>\n<test>conditional content</test>"""
			
			expectedResult = tree.ECTreeNode()
			expectedResult.addChild(operations.ECUnconditionalBlockNode("#!gbuild\n"))
			expectedResult.addChild(definitions.ECConfigDefinition('config', {'name':'test'}))
			expectedResult.addChild(operations.ECUnconditionalBlockNode("\n"))
			conditional = operations.ECConditionalBlockNode('test')
			conditional.addChild(operations.ECUnconditionalBlockNode("conditional content"))
			expectedResult.addChild(conditional)
			
			self.runVector(vector, expectedResult)
	
	templateFileSuite = unittest.makeSuite(ECProjectTemplateFileUnitTest)
	suite = unittest.TestSuite()
	suite.addTests((templateFileSuite))
	return suite

# Run unit tests when this source file is executed directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())

