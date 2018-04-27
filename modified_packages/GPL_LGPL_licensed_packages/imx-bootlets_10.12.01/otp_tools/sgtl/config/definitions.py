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
import model

# XXX need to add a method to definition nodes to perform semantic checks

def isAllWhitespace(s):
	"""Returns True if s contains only whitespace."""
	for c in s:
		if not c in ' \t\r\n':
			return False
	return True

class ECDefinitionNode(tree.ECTreeNode):
	"""Base class for object definition nodes. Definition nodes serve two purposes. First,
	they encapsulate the rules for how to parse the XML-style elements for a given
	entity. And second, they provide a place to store attributes and other data extracted
	from the XML-style elements.
	
	The ECDefinitionNode class has code to handle description elements."""
	
	def __init__(self, name, attributes):
		tree.ECTreeNode.__init__(self)
		self._inDescription = False
		self._description = ""
		self._elementName = ""
	
	def handleContent(self, data):
		if self._inDescription:
			self._description += data
		elif not isAllWhitespace(data):
			raise Exception("Unexpected content")
	
	def handleOpenElement(self, name, attributes):
		if name == 'description' and not self._inDescription:
			self._inDescription = True
		else:
			raise Exception("Unexpected element '" + name + "'")
	
	def handleCloseElement(self, name):
		if name == 'description' and self._inDescription:
			self._inDescription = False
		elif name == self._elementName:
			return True
	
	def getDescription(self):
		return self._description
	
	def createModelObject(self):
		return None
	
	def classSpecificFormat(self):
		return self._elementName
	
	def __eq__(self, other):
		return (tree.ECTreeNode.__eq__(self, other) and (self._elementName == other._elementName)
			and (self._description == other._description))
	
	def getYesNoAttributeValue(self, attributes, name, default):
		if not attributes.has_key(name):
			return default
		value = attributes[name]
		if value == 'yes':
			return True
		elif value == 'no':
			return False
		else:
			raise Exception("Invalid value for '" + name + "' attribute")
	
class ECNamedObjectDefinition(ECDefinitionNode):
	"""Base class for any definition node that supports a 'name' attribute."""
	
	def __init__(self, name, attributes):
		ECDefinitionNode.__init__(self, name, attributes)
		self._name = None
		
		if attributes.has_key('name'):
			self._name = attributes['name']
	
	def getName(self):
		return self._name
	
	def classSpecificFormat(self):
		return self._name
	
	def __eq__(self, other):
		return (ECDefinitionNode.__eq__(self, other) and (self._name == other._name))

class ECConfigDefinition(ECNamedObjectDefinition):
	"""Definition node class for 'config' elements. Configurations can have 'property'
	elements defined within them."""
	
	def __init__(self, name, attributes):
		ECNamedObjectDefinition.__init__(self, name, attributes)
		self._elementName = "config"
		self._isDefault = self.getYesNoAttributeValue(attributes, 'default', False)

	def getName(self):
		return self._name
	
	def getIsDefault(self):
		return self._isDefault
	
	def handleOpenElement(self, name, attributes):
		if name == 'property' and not self._inDescription:
			# Create the new property definition node and return it so it becomes the
			# current element object. We pass ourself as the scope node so the property
			# is put within this configuration's scope.
			return ECPropertyDefinition(name, attributes, self)
		else:
			return ECNamedObjectDefinition.handleOpenElement(self, name, attributes)
	
	def createModelObject(self):
		return model.ECConfig(self._name, self._description, self._isDefault)
	
	def classSpecificFormat(self):
		fmt = self._name
		if self._isDefault:
			fmt += " (default)"
		return fmt
	
	def __eq__(self, other):
		return (ECNamedObjectDefinition.__eq__(self, other) and (self._isDefault == other._isDefault))

class ECProjectDefinition(ECNamedObjectDefinition):
	"""Definition node class for 'project' elements."""
	
	def __init__(self, name, attributes):
		ECNamedObjectDefinition.__init__(self, name, attributes)
		self._elementName = "project"
	
	def createModelObject(self):
		return model.ECProject(self._name, self._description)
	
class ECExpressionDefinitionNode(ECNamedObjectDefinition):
	"""Abstract base definition node class for elements that accept boolean expressions."""
	
	def __init__(self, name, attributes):
		ECNamedObjectDefinition.__init__(self, name, attributes)
		self._elementName = "define"
		self._expression = ""

	def handleContent(self, data):
		if self._inDescription:
			self._description += data
		else:
			self._expression += data
	
	def getExpression(self):
		return self._expression
	
	def classSpecificFormat(self):
		name = self._name
		if not name:
			name = "<anon>"
		expr = self._expression
		if not expr:
			expr = "<none>"
		return name + " '" + expr + "'"

class ECConditionalDefinition(ECExpressionDefinitionNode):
	"""Definition node class for 'define' elements."""
	
	def __init__(self, name, attributes):
		ECExpressionDefinitionNode.__init__(self, name, attributes)
		self._elementName = "define"
	
	def createModelObject(self):
		return model.ECConditional(self._name, self._description, self._expression)

class ECRequirementDefinition(ECExpressionDefinitionNode):
	"""Definition node class for 'require' elements."""
	
	def __init__(self, name, attributes):
		ECExpressionDefinitionNode.__init__(self, name, attributes)
		self._elementName = "require"
	
	def createModelObject(self):
		return model.ECRequirement(self._name, self._description, self._expression)

class ECPropertyDefinition(ECNamedObjectDefinition):
	"""Definition node class for 'property' elements."""
	
	def __init__(self, name, attributes, scope=None):
		ECNamedObjectDefinition.__init__(self, name, attributes)
		self._elementName = "property"
		self._value = ""
		self._scope = scope
		self._isOverridable = self.getYesNoAttributeValue(attributes, 'overrideable', True)
	
	def handleContent(self, data):
		if self._inDescription:
			self._value += data
		else:
			self._value += data
	
	def getValue(self):
		return self._value
	
	def getScope(self):
		"""Will return None if the property is global."""
		return self._scope
	
	def isOverridable(self):
		return self._isOverridable
	
	def createModelObject(self):
		# Assign the scope name to our scope node's name (the configuration name).
		scopeName = None
		if self._scope:
			scopeName = self._scope.getName()
		return model.ECProperty(self._name, self._description, self._value, scopeName, self._isOverridable)

# Returns a suite of all test cases in this source file.
def suite():
	# Unit test for isAllWhitespace.
	class WhitespaceUnitTest(unittest.TestCase):
		def test_whitespace(self):
			self.assertEqual(isAllWhitespace("hello"), False)
			self.assertEqual(isAllWhitespace(""), True)
			self.assertEqual(isAllWhitespace("hello there"), False)
			self.assertEqual(isAllWhitespace(" "), True)
			self.assertEqual(isAllWhitespace(" \t"), True)
			self.assertEqual(isAllWhitespace(" \t\r\n"), True)
			self.assertEqual(isAllWhitespace("\r\n\r\n"), True)
			self.assertEqual(isAllWhitespace("                 "), True)
	
	# Unit tests.
	class DefinitionUnitTest(unittest.TestCase):
		def test_config_def(self):
			node = ECConfigDefinition('config', {'name':'foo'})
			self.assertEqual(node.getName(), 'foo')
			self.assertEqual(node.getIsDefault(), False)
			self.assertEqual(node.getDescription(), '')
			
			node = ECConfigDefinition('config', {'name':'bar', 'default':'no'})
			self.assertEqual(node.getName(), 'bar')
			self.assertEqual(node.getIsDefault(), False)
			
			node = ECConfigDefinition('config', {'name':'baz', 'default':'yes'})
			self.assertEqual(node.getName(), 'baz')
			self.assertEqual(node.getIsDefault(), True)
			
			node = ECConfigDefinition('config', {'name':'foo'})
			node.handleOpenElement('description', {})
			node.handleContent('yo baby')
			node.handleCloseElement('description')
			self.assertEqual(node.getName(), 'foo')
			self.assertEqual(node.getIsDefault(), False)
			self.assertEqual(node.getDescription(), 'yo baby')
	
	whitespaceSuite = unittest.makeSuite(WhitespaceUnitTest)
	definitionSuite = unittest.makeSuite(DefinitionUnitTest)
	suite = unittest.TestSuite()
	suite.addTests((whitespaceSuite, definitionSuite))
	return suite

# Run unit tests when this source file is executed directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
