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
import sys, os

parentModulePath = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
if not parentModulePath in sys.path:
	sys.path.append(parentModulePath)
from utility import bufferedstream

__all__ = ["PropertyScope", "suite"]

# Characters that are valid in a property name.
PROPERTY_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"

class PropertyScope:
	def __init__(self, parent=None):
		self._parent = parent
		self._properties = {}
	
	def setProperty(self, name, value):
		self._properties[name] = value
	
	def setProperties(self, props):
		for name in props.keys():
			self._properties[name] = props[name]
	
	# If there is no property with the name, the parent is asked. If there is
	# no parent, the empty string is returned.
	def getProperty(self, name):
		if self._properties.has_key(name):
			return self._properties[name]
		elif self._parent:
			return self._parent.getProperty(name)
		else:
			return ""
	
	def getProperties(self):
		return self._properties
	
	def hasProperty(self, name):
		return self._properties.has_key(name)
	
	def deleteProperty(self, name):
		if self._properties.has_key(name):
			del self._properties[name]
	
	def substitutePropertiesInString(self, input):
		while True:
			inProperty = False
			name = ""
			output = ""
			didSubstitute = False
			doneWithName = False
			invalidProperty = False
			
			stream = bufferedstream.BufferedStringCharacterStream(input)
			while True:
				c = stream.get()
				if c == None:
					break
				if inProperty:
					if c == '#' and stream.peek() == '#':
						stream.get(1) # absorb the second '#'
						if not invalidProperty:
							output += self.getProperty(name)
							didSubstitute = True
						inProperty = False
					elif c in ' \t':
						doneWithName = True
					elif c in PROPERTY_CHARS:
						invalidProperty = doneWithName or invalidProperty
						name += c
					else:
						invalidProperty = True
				elif c == '#' and stream.peek() == '#':
					stream.get(1) # absorb the second '#'
					# skip whitespace
					while True:
						c = stream.get()
						if not c in ' \t':
							stream.put(c)
							break
					inProperty = True
					doneWithName = False
					invalidProperty = False
					name = ""
				else:
					output += c
		
			# handle property at end of input
			if inProperty and not nameHasParens:
				output += self.getProperty(name)
				didSubstitute = True
			
			# perform recursive substitution if any properties were substituted
			if didSubstitute:
				input = output
			else:
				break
		
		return output
	
	def dump(self):
		print self._properties

# Returns a suite of all test cases in this source file.
def suite():
	# Unit test for properties module.
	class PropertiesUnitTest(unittest.TestCase):
		def test_basic(self):
			scope = PropertyScope()
			scope.setProperty("FOO", "monkey")
			self.assertEqual(scope.substitutePropertiesInString("##FOO##"), "monkey")
			self.assertEqual(scope.substitutePropertiesInString("aaa## FOO ##"), "aaamonkey")
			self.assertEqual(scope.substitutePropertiesInString("##FOO xx## aaa"), " aaa")
			self.assertEqual(scope.substitutePropertiesInString("## 	FOO 	##"), "monkey")
			self.assertEqual(scope.substitutePropertiesInString("aaa##FOO##"), "aaamonkey")
			self.assertEqual(scope.substitutePropertiesInString("##FOO##aaa"), "monkeyaaa")
			self.assertEqual(scope.substitutePropertiesInString("The ##FOO## jumps."), "The monkey jumps.")
			self.assertEqual(scope.substitutePropertiesInString("The ## FOO ## jumps."), "The monkey jumps.")
			
			testInput = "-##FOO##-## FOO##-##BAR##-##BAR ##-"
			scope.setProperty("BAR", "funny")
			self.assertEqual(scope.substitutePropertiesInString(testInput), "-monkey-monkey-funny-funny-")
			
			scope.setProperty("BAR", "honey")
			self.assertEqual(scope.substitutePropertiesInString(testInput), "-monkey-monkey-honey-honey-")
			
			scope.deleteProperty("BAR")
			self.assertEqual(scope.substitutePropertiesInString(testInput), "-monkey-monkey---")
			
			scope.setProperty("BLEEP", "frick")
			testInput = "super##BLEEP##ingdiliocious"
			self.assertEqual(scope.substitutePropertiesInString(testInput), "superfrickingdiliocious")
			
			testInput = "super## BLEEP ##ingdiliocious"
			self.assertEqual(scope.substitutePropertiesInString(testInput), "superfrickingdiliocious")
		
		def test_recursive_properties(self):
			scope = PropertyScope()
			scope.setProperty("PROPA", "power")
			scope.setProperty("PROPB", "glory")
			scope.setProperty("PROPC", "##PROPA##==##PROPB##")
			self.assertEqual(scope.substitutePropertiesInString("##PROPA##"), "power")
			self.assertEqual(scope.substitutePropertiesInString("##PROPB##"), "glory")
			self.assertEqual(scope.substitutePropertiesInString("##PROPC##"), "power==glory")
		
		def test_scopes(self):
			parent = PropertyScope()
			child = PropertyScope(parent)
			
			parent.setProperty("TEST", "yada, yada, yada...")
			parent.setProperty("BUBBA", "bUURRRP!")
			child.setProperty("TEST", "oh yeah?")
			
			testInput = "This is a ##TEST## of properties"
			self.assertEqual(parent.substitutePropertiesInString(testInput), "This is a yada, yada, yada... of properties")
			self.assertEqual(child.substitutePropertiesInString(testInput), "This is a oh yeah? of properties")
			
			self.assertEqual(parent.getProperty("BUBBA"), "bUURRRP!")
			self.assertEqual(child.getProperty("BUBBA"), "bUURRRP!")
			
			testInput = "Bubba said '##BUBBA##'."
			self.assertEqual(parent.substitutePropertiesInString(testInput), "Bubba said 'bUURRRP!'.")
			self.assertEqual(child.substitutePropertiesInString(testInput), "Bubba said 'bUURRRP!'.")
		
		def test_has(self):
			scope = PropertyScope()
			self.assert_(not scope.hasProperty("APROPERTY"))
			scope.setProperty("APROPERTY", "howdy")
			self.assert_(scope.hasProperty("APROPERTY"))
			scope.deleteProperty("APROPERTY")
			self.assert_(not scope.hasProperty("APROPERTY"))
	
	suite = unittest.makeSuite(PropertiesUnitTest)
	return suite

# Execute unit tests if we're called directly from the command line.
if __name__ == "__main__":
	unittest.TextTestRunner(verbosity=2).run(suite())

