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
import expressions

# XXX Make these model classes into more than just containers. Move the application logic here.

anonymousCount = 0

class ECModelObject:
	"""Base class for all config package model objects. All model objects are expected
	to have a name and an optional description."""
	
	def __init__(self, name, description=""):
		self._name = name
		self._description = description
	
	def getName(self):
		return self._name
	
	def getDescription(self):
		return self._description

class ECProject(ECModelObject):
	pass

class ECConfig(ECModelObject):
	"""Model object for a configuration."""
	
	def __init__(self, name, description, isDefault):
		ECModelObject.__init__(self, name, description)
		self._isDefault = isDefault
		
	def isDefault(self):
		return self._isDefault

class ECConditional(ECModelObject):
	"""Model object for a conditional variable."""
	
	def __init__(self, name, description, expression):
		ECModelObject.__init__(self, name, description)
		self._expression = expression
	
	def getExpression(self):
		return self._expression
	
	def getExpressionTree(self):
		parser = expressions.ECExpressionParser(self._expression)
		tree = parser.parseExpr()
		return tree

class ECRequirement(ECModelObject):
	"""Model object class for a requirement rule."""
	
	def __init__(self, name, description, expression):
		ECModelObject.__init__(self, name, description)
		self._expression = expression
		
		# Assign anonymous name.
		if not self._name:
			global anonymousCount
			anonymousCount += 1
			self._name = "$rule" + str(anonymousCount)
	
	def getExpression(self):
		return self._expression
	
	def getExpressionTree(self):
		parser = expressions.ECExpressionParser(self._expression)
		tree = parser.parseExpr()
		return tree

class ECProperty(ECModelObject):
	"""Model object class for a property. The property's scope name will be None
	if the property is global. Otherwise the scope name will be the name of one of
	the configurations."""
	
	def __init__(self, name, description, value, scopeName=None, isOverridable=True):
		ECModelObject.__init__(self, name, description)
		self._value = value
		self._scope = scopeName
		self._isOverridable = isOverridable
	
	def getValue(self):
		return self._value
	
	def getSubstitutedValue(self, otherProperties):
		return self._value
	
	def isGlobal(self):
		return self._scope == None
	
	def getScopeName(self):
		return self._scope
	
	def isOverridable(self):
		return self._isOverridable

# Returns a suite of all test cases in this source file.
def suite():
	# Unit tests for ECProject.
	class ProjectUnitTest(unittest.TestCase):
		def test_project_model(self):
			p = ECProject('foo', 'description')
			self.assertEqual(p.getName(), 'foo')
			self.assertEqual(p.getDescription(), 'description')
	
	# Unit tests for ECConfig.
	class ConfigUnitTest(unittest.TestCase):
		def test_config_model(self):
			c = ECConfig('example', 'some descriptive text', True)
			self.assertEqual(c.getName(), 'example')
			self.assertEqual(c.getDescription(), 'some descriptive text')
			self.assertEqual(c.isDefault(), True)
	
	# Unit tests for ECConditional.
	class ConditionalUnitTest(unittest.TestCase):
		def test_conditional_model(self):
			c = ECConditional('name', 'desc', 'foo || bar')
			self.assertEqual(c.getName(), 'name')
			self.assertEqual(c.getDescription(), 'desc')
			self.assertEqual(c.getExpression(), 'foo || bar')
			
			tree = expressions.ECExpressionBinaryOp(expressions.ECExpressionVariable('foo'), expressions.OR_OP, expressions.ECExpressionVariable('bar'))
			self.assertEqual(c.getExpressionTree(), tree)
	
	# Unit tests for ECRequirement.
	class RequirementUnitTest(unittest.TestCase):
		def test_requirement_model(self):
			c = ECRequirement('name', 'desc', 'foo && !bar')
			self.assertEqual(c.getName(), 'name')
			self.assertEqual(c.getDescription(), 'desc')
			self.assertEqual(c.getExpression(), 'foo && !bar')
	
	# Unit tests for ECProperty.
	class PropertyUnitTest(unittest.TestCase):
		def test_property_model_global(self):
			c = ECProperty('name', 'desc', 'value')
			self.assertEqual(c.getName(), 'name')
			self.assertEqual(c.getDescription(), 'desc')
			self.assertEqual(c.getValue(), 'value')
			self.assertEqual(c.getScopeName(), None)
			self.assertEqual(c.isGlobal(), True)
			
		def test_property_model_scoped(self):
			c = ECProperty('name', 'desc', 'value', 'some_config')
			self.assertEqual(c.getName(), 'name')
			self.assertEqual(c.getDescription(), 'desc')
			self.assertEqual(c.getValue(), 'value')
			self.assertEqual(c.getScopeName(), 'some_config')
			self.assertEqual(c.isGlobal(), False)
	
	projectSuite = unittest.makeSuite(ProjectUnitTest)
	configSuite = unittest.makeSuite(ConfigUnitTest)
	conditionalSuite = unittest.makeSuite(ConditionalUnitTest)
	requirementSuite = unittest.makeSuite(RequirementUnitTest)
	propertySuite = unittest.makeSuite(PropertyUnitTest)
	suite = unittest.TestSuite()
	suite.addTests((projectSuite, configSuite, conditionalSuite, requirementSuite, propertySuite))
	return suite

# Run unit tests when this source file is executed directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())
