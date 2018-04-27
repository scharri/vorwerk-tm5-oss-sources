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
import unittest
import templatefile
import definitions
import model
import operations
import expressions
import properties
import shutil
import tempfile
import filecmp
import logging
#import sgtl.build.loghandler import DefaultLogHandler.log as sgtlLoghandler
#from sgtl.build.loghandler import DefaultLogHandler
from sgtl.build import loghandler
import pdb

class ECOutputContext:
	def __init__(self):
		pass

	def write(self, data):
		pass

class ECController(ECOutputContext):
	"""The controller class ties all the pieces of the embedded configuration
	package together. This is the API that external code will use.

	XXX try to refactor more of the application logic into the model classes
	XXX move the lists of model objects into the ECProject model class
	XXX separate self._conditionals into one dict for expressions and one for final values"""

	def __init__(self, projectTemplatePath):
		self._tree = None
		self._templatePath = projectTemplatePath
		self._outputFilePath = None
		self._outputFile = None
		self._temporaryFile = None
		self._loggingLevel = logging.WARN
		self._bLogTheConfig = False

		# model objects
		self._project = None
		self._configs = {}	# keys are lower-case
		self._defines = []
		self._properties = []
		self._propertyNames = {}	# keys are lower-case, values are obj
		self._rules= []

		# externally set values
		self._configValues = {}
		self._propertyValues = {}
		self._overwrite = "yes"

		# runtime values
		self._conditionals = {}
		self._propertyScope= properties.PropertyScope()
		self._didParse = False

	def setOneConfigValue(self, configName, boolValue):
		# Set conditional for config to a constant bool expression.
		if not configName in self._conditionals:
			self._configValues[configName] = boolValue

	def setConfigValues(self, configNames, boolValue):
		for name in configNames:
			self.setOneConfigValue(name, boolValue)

	def setLoggingLevel(self, loggingLevel):
		self._loggingLevel = loggingLevel
		rootLogger = logging.getLogger()
		rootLogger.setLevel( level=loggingLevel )

	def setBLogTheConfig(self, bLogTheConfig):
		self._bLogTheConfig = bLogTheConfig

	def setOnePropertyValue(self, propertyName, value):
		self._propertyValues[propertyName] = value

	def setOverwrite(self, overwrite):
		self._overwrite = overwrite.lower()

	def setPropertyValues(self, propertyValueDict):
		for name in propertyValueDict.keys():
			self.setOnePropertyValue(name, propertyValueDict[name])

	def setOutputFilePath(self, outputFilePath):
		"""Accepts a file name into which to write the output. The file will be opened
		with write access."""
		self._outputFilePath = outputFilePath

	def getProject(self):
		"""Returns the project model object."""
		return self._project

	def getConfigs(self):
		"""Returns a list of configuration objects."""
		return self._configs.values()
	
	def getConfigValues(self):
		"""Returns a dictionary of externally set config values."""
		return self._configValues
	
	def getPropertyValues(self):
		"""Returns a dictionary of externally set property values."""

	def parse(self):
		"""Converts the input project template into a tree of objects."""
		if not self._didParse:
			try:
				self._template = templatefile.ECProjectTemplateFile(self._templatePath)
				self._tree = self._template.parseTree()
				self._didParse = True
			except:
				Exception, "Could not parse template ", self._templatePath
				raise

	def createObjects(self):
		"""Scans the tree for object definition elements and creates the
		corresponding live objects.
		XXX need to remove the definition nodes after they are processed"""

		def createObjectsCallback(node):
			# We only care about definition nodes here.
			if not isinstance(node, definitions.ECDefinitionNode):
				return True

			# Create object instance from definition.
			obj = node.createModelObject()
			if not obj:
				return True

			#logging.debug( '%s->%s', str(node), str(obj))

			# File away the instance depending on its class.
			if isinstance(obj, model.ECProject):
				self._project = obj
			elif isinstance(obj, model.ECConfig):
				self._configs[obj.getName().lower()] = obj
			elif isinstance(obj, model.ECConditional):
				self._defines.append(obj)
			elif isinstance(obj, model.ECRequirement):
				self._rules.append(obj)
			elif isinstance(obj, model.ECProperty):
				self._properties.append(obj)
				self._propertyNames[obj.getName().lower()] = obj

			return True

		self._tree.visit(createObjectsCallback)

	def filterTree(self):
		"""Removes object definitions from the tree. Also removes unconditional blocks containing
		only whitespace that are situated between two object definitions.

		For both of these operations, we assume there are no definitions deeper than the first
		sibling chain."""

		# First scan for whitespace blocks between definitions.
		node = self._tree.getFirstChild()
		while True:
			prevNode = node.getPrev()
			nextNode = node.getNext()

			prevOK = True
			if prevNode:
				prevOK = isinstance(prevNode, definitions.ECDefinitionNode)
			nextOK = True
			if nextNode:
				nextOK = isinstance(nextNode, definitions.ECDefinitionNode)

			isWhitespace = isinstance(node, operations.ECUnconditionalBlockNode) and node.getContent().isspace()

			if isWhitespace and prevOK and nextOK:
				node.remove()

			if not nextNode:
				break
			else:
				node = nextNode

		# Now remove object definition nodes.
		node = self._tree.getFirstChild()
		while True:
			nextNode = node.getNext()
			if isinstance(node, definitions.ECDefinitionNode):
				node.remove()

			if not nextNode:
				break
			else:
				node = nextNode

	def validateTree(self):
		"""Checks the contents of the tree against the objects that have been defined.
		This is where conditional blocks are checked to make sure that a config has
		been defined for each one."""

		def validateCallback(node):
			if not isinstance(node, operations.ECConditionalBlockNode):
				return True

			# Raise an exception if the conditional block uses an undefined conditional.
			conditionalName = node.getConditional()
			if not self._conditionals.has_key(conditionalName):
				raise Exception("Conditional block for undefined conditional '" + conditionalName + "'")

			return True

		self._tree.visit(validateCallback)

	def createConditionals(self):
		"""This method fills in the conditional variable dictionary. After returning, the
		dictionary values will be set to expression tree nodes."""

		# Assign conditional values for configs set externally.
		for name in self._configValues.keys():
			lowerName = name.lower()
			if not self._configs.has_key(lowerName):
				raise Exception("Value set for undefined configuration '" + name + "'")

			# Get config object so we can have the properly-cased name.
			config = self._configs[lowerName]
			self._conditionals[config.getName()] = expressions.ECExpressionConstant(self._configValues[name])

		# Assign default values for configurations that haven't been explicitly set already.
		for config in self._configs.values():
			# Assign default value (True/False) if the config hasn't already been
			# given a value.
			name = config.getName()
			if not name in self._conditionals:
				self._conditionals[name] = expressions.ECExpressionConstant(config.isDefault())

		# Create conditional variables from conditional definitions.
		for define in self._defines:
			name = define.getName()
			self._conditionals[name] = define.getExpressionTree()

		# Create conditional variables from requirement rules.
		for rule in self._rules:
			name = rule.getName()
			self._conditionals[name] = rule.getExpressionTree()

		#print self._conditionals

	def createProperties(self):
		"""Examines the property model objects, removes those that are within a non-selected
		configuration, and adds the remaining ones to the property scope/dictionary."""
		nonOverridableProperties = []

		# Set values for all global properties first.
		for property in [p for p in self._properties if p.isGlobal()]:
			# Set this property's value in the global property scope object.
			self._propertyScope.setProperty(property.getName(), property.getValue())

			if not property.isOverridable():
				nonOverridableProperties.append(property.getName())

		# Now examine the scoped properties so they can override the globals that have
		# already been set.
		for property in [p for p in self._properties if not p.isGlobal()]:
			# Set this property if it belongs to a valid configuration that is selected.
			scopeName = property.getScopeName()
			if self._configs.has_key(scopeName.lower()) and self._conditionals[scopeName]:
				self._propertyScope.setProperty(property.getName(), property.getValue())

				if not property.isOverridable():
					nonOverridableProperties.append(property.getName())

		# Now process externally set properties.
		for name in self._propertyValues.keys():
			# Get value that was passed in externally.
			value = self._propertyValues[name]

			# Look up the actual name (correct case) of this property. If there is not a
			# property defined with this name, then we'll set the value in the property scope
			# use the name passed in externally, as is.
			if self._propertyNames.has_key(name.lower()):
				name = self._propertyNames[name.lower()].getName()

			# Print a warning and skip this one if the user is trying to set a value on a
			# property that 1) is global or in a selected config and 2) is non-overridable.
			if self._propertyScope.hasProperty(name) and name in nonOverridableProperties:
				logging.warning( "Cannot override property '" + name + "'" )
				continue

			# Set property value in the scope.
			self._propertyScope.setProperty(name, value)

		#self._propertyScope.dump()

	def evaluateConditionals(self):
		"""This is where expressions in the conditional dictionary are evaluated and a
		single boolean result is produced. The entries in the conditional dictionary are
		replaced with the boolean result of the original expression."""

		#print "conditionals:",self._conditionals
		for name in self._conditionals.keys():
			expr = self._conditionals[name]
			self._conditionals[name] = expr.evaluate(self)

	def getVariableValue(self, identifier):
		if not self._conditionals.has_key(identifier):
			raise Exception("unknown conditional variable: " + identifier)

		conditionalValue = self._conditionals[identifier]
		if isinstance(conditionalValue, expressions.ECExpression):
			return conditionalValue.evaluate(self)
		else:
			return conditionalValue

	def evaluateRules(self):
		"""Test all requirement rules to make sure they evaluate to True.

		Conditional variables must have already been evaluated into booleans before
		this method is called."""

		for rule in self._rules:
			ruleName = rule.getName()
			if not self._conditionals.has_key(ruleName):
				raise Exception("no conditional variable for rule: " + ruleName)
			if not self._conditionals[ruleName]:
				raise Exception("Requirement rule '%s' failed: %s'" % (ruleName, rule.getDescription()))

	def generateOutput(self):
		"""Scan the tree and executes operation nodes to generate output."""

		def generateCallback(node):
			# We only care about operation nodes here.
			if not isinstance(node, operations.ECOperationNode):
				return True

			# Only execute conditional blocks and their children if the conditional
			# to which they are tied is True.
			if isinstance(node, operations.ECConditionalBlockNode):
				conditionalName = node.getConditional()
				if not self._conditionals.has_key(conditionalName):
					return False
				if not self._conditionals[conditionalName]:
					return False

			node.execute(self)
			return True

		self._tree.visit(generateCallback)

	def run(self):
		from sgtl.build import log
		if self._bLogTheConfig:
			log("\nProject file     = " + self._outputFilePath )
			log( "Configs: " + str(self._configValues) )
			log( "Property values: " + str(self._propertyValues) + "\n" )

		if ( (self._overwrite == "no") and os.path.exists(self._outputFilePath) ):
			# Don't overwrite existing files.
			logging.info("Skipped configuring '" + self._outputFilePath + "' because the overwrite option is off.")
			return

		# If we got to here, then _overwrite is not False.
		# It is permissible to overwrite the output file, under
		# the right circumstances.

		#
		# Create a temporary file object for the output.
		#
		(temporaryFd, temporaryFilePath) = tempfile.mkstemp()	# Have to open the file using mkstemp()
		#														# rather than TemporaryFile(), so that
		#														# the file won't be deleted automatically
		#														# if we close it.
		#														#
		os.close(temporaryFd)									# Now create...
		self._temporaryFile = file(temporaryFilePath, 'w+b')	# ...the file object.

		try:

			# Here's all the work to make the configured file.

			try:
				self.parse()
				self.createObjects()
				self.createConditionals()
				self.filterTree()
				self.validateTree()
				self.evaluateConditionals()
				self.createProperties()
				self.evaluateRules()
				self.generateOutput()

			finally:
				# Close the temporary file to which the configured output
				# was written.  The file needs to be closed now, so that it
				# can be re-opened for reading next.
				self._temporaryFile.close()

			# Logic for the "Diff" overwrite criterion.
			# This implementation is verbose, but it is easy to follow.
			if ( os.path.exists(self._outputFilePath) ):

				# There's an existing output file, so we
				# have to check to see when it should be overwritten.

				if ( self._overwrite == "update" ):

					logging.debug( '***Update-mode' )
					# The output file should be overwritten if the new
					# configured output is different.
					if ( False == filecmp.cmp( self._temporaryFile.name, self._outputFilePath ) ):
						# The new configured output is different.
						shutil.copyfile(self._temporaryFile.name, self._outputFilePath)

					else:
						logging.info("Skipped configuring '" + self._outputFilePath + "' because the existing file is identical.")


				else:

					logging.debug( '***Overwrite-mode' )
					# The output file should be overwritten at will.
					shutil.copyfile(self._temporaryFile.name, self._outputFilePath)
					logging.info("Configured project: " + self._outputFilePath)


			else:

				logging.debug( '***Create-mode' )
				# There's no existing output file.  Make one.
				shutil.copyfile(self._temporaryFile.name, self._outputFilePath)

		finally:
			# Delete the temporary file, now that we are finished with it.
			os.remove(self._temporaryFile.name)



	def write(self, data):
		if self._temporaryFile:
			self._temporaryFile.write(self._propertyScope.substitutePropertiesInString(data))

# Returns a suite of all test cases in this source file.
def suite():
	# Unit test for ECController class.
	class ECControllerUnitTest(unittest.TestCase):
		def test_controller(self):
			path = os.path.join('vectors', 'small.tgpj')
			c = ECController(path)
			outPath = os.path.join('vectors', 'outtest.gpj')
			c.setOutputFilePath(outPath)
			c.run()

	return unittest.makeSuite(ECControllerUnitTest)

# Run unit tests when this source file is executed directly from the command
# line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())


