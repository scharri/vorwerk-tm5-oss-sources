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

__all__ = ["ECTreeNode", "suite"]

class ECTreeNode:
    """Instances of this class form trees of linked lists. The methods of this tree class are
    optimised for building a tree from a lexically sequential input."""
    
    def __init__(self):
        self._next = None
        self._prev = None
        self._firstChild = None
        self._parent = None
    
    def getNext(self):
        return self._next
    
    def getPrev(self):
        return self._prev
    
    def setNext(self, newNext):
        self._next = newNext
        newNext._prev = self
    
    def setPrev(self, newPrev):
        self._prev = newPrev
        newPrev._next = self
        
    def getFirstChild(self):
        return self._firstChild
    
    def setFirstChild(self, child):
        self._firstChild = child
        child._parent = self
    
    def getFirstSibling(self):
        sibling = self
        while sibling._prev:
            sibling = sibling._prev
        return sibling
        
    def getLastSibling(self):
        sibling = self
        while sibling._next:
            sibling = sibling._next
        return sibling
    
    def addSibling(self, newSibling):
        """Adds newSibling to the end of the linked list of siblings of which self is a part."""
        lastSibling = self.getLastSibling()
        lastSibling.setNext(newSibling)
    
    def addChild(self, child):
        """Adds child to the end of the child list of self. If there are no other
        children yet, child becomes the first child node."""
        # Handle when this is the first child for us.
        if not self._firstChild:
            self.setFirstChild(child)
        else:
            # At least one other child. Find the end of the child list.
            self._firstChild.addSibling(child)
    
    def getParent(self):
        """Finds the first sibling and returns its parent field. The parent is
        always and only attached to the first sibling."""
        # If we're the first in a list and have a parent set, return it.
        if not self._prev and self._parent:
            return self._parent
        
        # Otherwise search for our first sibling and return her parent, if any.
        return self.getFirstSibling()._parent
    
    def remove(self):
        """Removes self from the tree of which it is a member. Our children still retain
        us as the parent. If this object is the first child of its parent, the new first
        child will be set to our next sibling."""
        
        if self._prev:
            self._prev._next = self._next
            if self._next:
                self._next._prev = self._prev
        elif not self._prev and self._parent:
            self._parent._firstChild = self._next
            if self._next:
                self._next._prev = None
                self._next._parent = self._parent
            self._parent = None
        
        self._prev = None
        self._next = None
    
    def visit(self, fn):
        """Call fn for every node on this tree."""
        thisOne = self.getFirstSibling()
        while True:
            # Visit any children if the callback returns True.
            if fn(thisOne) and thisOne._firstChild:
                thisOne._firstChild.visit(fn)
            
            # Either break the loop or continue with the next sibling.
            if not thisOne._next:
                break
            thisOne = thisOne._next
    
    def classSpecificFormat(self):
        """Returns a string containing information particular to the subclass. This
        string is placed in the result of genericFormat()."""
        return ""
    
    def formatTree(self, indentLevel):
        """Combines the results of genericFormat() for the siblings and children of self
        to produce a readable description of the tree. The indentLevel argument is used to
        indent child nodes further to the right the deeper they get."""
        result = ""
        thisOne = self
        while True:
            result += '\t' * indentLevel + str(thisOne)
                
            if thisOne._firstChild:
                result += '{\n' + thisOne._firstChild.formatTree(indentLevel + 1) + '\n' + '\t' * indentLevel + '}'
            
            if not thisOne._next:
                break
            thisOne = thisOne._next
            
            result += '\n'
        return result
    
    def __str__(self):
        """Formats a string describing this object instance. The first part is always the
        same for all tree nodes, and the second part comes from classSpecificFormat()."""
        classData = self.classSpecificFormat()
        if len(classData):
            classData = " " + classData
        return "<%s:0x%x%s>" % (str(self.__class__), id(self), classData)
    
    def __eq__(self, other):
        if not (self.__class__ == other.__class__):
            return False
        if not (self._next == other._next):
            return False
        #if not (self._prev == other._prev):
        #   return False
        if not (self._firstChild == other._firstChild):
            return False
        #if not (self._parent == other._parent):
        #   return False
        return True

# Returns a suite of all test cases in this source file.
def suite():
    class BoolTreeNode(ECTreeNode):
        def __init__(self, value):
            ECTreeNode.__init__(self)
            self._value = value
        
        def getValue(self):
            return self._value
        
        def setValue(self, newValue):
            self._value = newValue
        
    # Unit tests.
    class TreeUnitTest(unittest.TestCase):
        def test_siblings(self):
            a = ECTreeNode()
            b = ECTreeNode()
            c = ECTreeNode()
            
            a.addSibling(b)
            a.addSibling(c)
            
            self.assertEqual(a.getNext(), b)
            self.assertEqual(b.getNext(), c)
            
            self.assertEqual(a.getLastSibling(), c)
            self.assertEqual(b.getLastSibling(), c)
            self.assertEqual(c.getLastSibling(), c)
            
            self.assertEqual(c.getFirstSibling(), a)
            self.assertEqual(b.getFirstSibling(), a)
            self.assertEqual(a.getFirstSibling(), a)
    
        def test_children(self):
            a = ECTreeNode()
            b = ECTreeNode()
            c = ECTreeNode()
            d = ECTreeNode()
            e = ECTreeNode()
            f = ECTreeNode()
            
            a.addSibling(b)
            b.addSibling(c)
            
            b.addChild(d)
            self.assertEqual(b.getFirstChild(), d)
            self.assertEqual(d.getParent(), b)
            
            b.addChild(e)
            self.assertEqual(b.getFirstChild().getLastSibling(), e)
            self.assertEqual(e.getFirstSibling(), d)
            self.assertEqual(e.getParent(), b)
            
            d.addSibling(f)
            self.assertEqual(f.getParent(), b)
            self.assertEqual(f.getFirstSibling(), d)
            
        def test_remove(self):
            a = ECTreeNode()
            b = ECTreeNode()
            c = ECTreeNode()
            d = ECTreeNode()
            e = ECTreeNode()
            f = ECTreeNode()
            
            a.addSibling(b)
            a.addSibling(c)
            a.addSibling(d)
            
            b.remove()
            self.assertEqual(a.getNext(), c)
            self.assertEqual(c.getPrev(), a)
            
            c.addChild(e)
            c.addChild(f)
            
            f.remove()
            self.assertEqual(c.getFirstChild(), e)
            self.assertEqual(e.getNext(), None)
            
            c.addChild(f)
            e.remove()
            self.assertEqual(c.getFirstChild(), f)
            self.assertEqual(f.getPrev(), None)
        
        def test_visit(self):
            a = BoolTreeNode(True)
            b = BoolTreeNode(False)
            c = BoolTreeNode(True)
            d = BoolTreeNode(True)
            x = BoolTreeNode(True)
            y = BoolTreeNode(True)
            
            a.addChild(x)
            a.addSibling(c)
            b.addChild(y)
            b.addSibling(d)
            
            global count
            
            def testfn(q):
                global count
                count += 1
                return q.getValue()
            
            count = 0
            a.visit(testfn)
            self.assertEqual(count, 3)
            
            count = 0
            b.visit(testfn)
            self.assertEqual(count, 2)
    
    suite = unittest.makeSuite(TreeUnitTest)
    return suite

# Run unit tests when this source file is executed directly from the command line.
if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite())
