#!/bin/env python

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

import sys, os, time, struct
import unittest

# Constants

HEADER_SIZE = 52 # Size of the first block header that contains the signature and versions.
SIGNATURE_SIZE = 4 # Size of the header signature.
VERSION_SIZE = 4 * 3 # Size of a version field.

SIGNATURE_OFFSET = 20
PRODUCT_VERSION_OFFSET = SIGNATURE_OFFSET + SIGNATURE_SIZE
COMPONENT_VERSION_OFFSET = PRODUCT_VERSION_OFFSET + VERSION_SIZE

def convertBCDToDecimal(fieldValue):
    """Converts a right aligned BCD number to a decimal value."""
    result = (((fieldValue & 0xf000) >> 12) * 1000)
    result += (((fieldValue & 0x0f00) >> 8) * 100)
    result += (((fieldValue & 0x00f0) >> 4) * 10)
    result += (fieldValue & 0x000f)
    return result
    
def convertDecimalToBCD(value):
    """Converts a decimal value to a right aligned BCD number."""
    currentValue = value
    bitPosition = 0
    versionValue = 0
    while currentValue > 0:
        digit = currentValue % 10
        versionValue = versionValue | (digit << bitPosition)
        
        bitPosition += 4
        currentValue = currentValue / 10
    
    return versionValue

class VersionedFile(file):
    """Subclass of the standard file class, extended to support files that have
    a header with the product and component version fields. It provides method 
    to both read and write these versions. There is also a method to access the 
    signature field.
    
    Note that for this class to work correctly, the file must be opened in
    binary mode."""
    
    def readSignature(self):
        """x"""
        
        header = self.readHeader()
        
        # extract signature
        signatureData = header[SIGNATURE_OFFSET:PRODUCT_VERSION_OFFSET]
        return signatureData
    
    def readProductVersion(self):
        return self.readVersion(PRODUCT_VERSION_OFFSET)
    
    def readComponentVersion(self):
        return self.readVersion(COMPONENT_VERSION_OFFSET)
    
    def writeProductVersion(self, version):
        """Accepts a tuple consisting of (major, minor, release) and writes
        it into the file at the product version field position."""
        self.writeVersion(PRODUCT_VERSION_OFFSET, version)
        
    def writeComponentVersion(self, version):
        """Accepts a tuple consisting of (major, minor, release) and writes
        it into the file at the component version field position."""
        self.writeVersion(COMPONENT_VERSION_OFFSET, version)
    
    def readVersion(self, headerOffset):
        """Returns a tuple containing the (major, minor, release) versions found
        at the given offset of this file."""
        
        header = self.readHeader()
        
        # extract version
        versionData = header[headerOffset:headerOffset+VERSION_SIZE]
        versionFields = struct.unpack('>HHHHHH', versionData) # treat as big endian shorts
        
        major = convertBCDToDecimal(versionFields[0])
        minor = convertBCDToDecimal(versionFields[2])
        release = convertBCDToDecimal(versionFields[4])
        
        return (major, minor, release)
        
    def writeVersion(self, headerOffset, version):
        # convert each field to BCD
        major = convertDecimalToBCD(version[0])
        minor = convertDecimalToBCD(version[1])
        release = convertDecimalToBCD(version[2])
        
        # convert to a big endian string
        versionData = struct.pack('>HHHHHH', major, 0, minor, 0, release, 0)
        
        # write to the file
        self.seek(headerOffset)
        self.write(versionData)
        self.flush()
        
    def readHeader(self):
        """Reads and returns the entire header from the file. If the file is too
        short, an Exception will be raised."""
        
        # read the header
        self.seek(0)
        header = self.read(HEADER_SIZE)
        if len(header) < HEADER_SIZE:
            raise Exception("file is too short to contain the header")
        
        return header

class VersionedFileUnitTest(unittest.TestCase):
    """Unit test for the VersionedFile class and related utility functions."""
    
    def test_read(self):
        f = VersionedFile("player.rsc", "rb")
        self.assertEqual(f.readProductVersion(), (3, 15, 83))
        self.assertEqual(f.readComponentVersion(), (8, 3, 81))
        self.assertEqual(f.readSignature(), 'RSRC')
        f.close()
    
    def test_convertToDecimal(self):
        bcd = 0x7890
        decimal = convertBCDToDecimal(bcd)
        self.assertEqual(decimal, 7890)
    
    def test_convertToBCD(self):
        decimal = 1234
        bcd = convertDecimalToBCD(decimal)
        self.assertEqual(bcd, 0x1234)
    
    def test_write(self):
        import shutil
        
        # make a copy
        copyPath = "player_copy.rsc"
        if os.path.exists(copyPath):
            os.unlink(copyPath)
        shutil.copy2("player.rsc", copyPath)
        
        # check contents before modifying
        f = VersionedFile(copyPath, "r+b")
        self.assertEqual(f.readProductVersion(), (3, 15, 83))
        self.assertEqual(f.readComponentVersion(), (8, 3, 81))
        self.assertEqual(f.readSignature(), 'RSRC')
        
        # modify the copy and test results
        vers1 = (4, 100, 789)
        f.writeProductVersion(vers1)
        self.assertEqual(f.readProductVersion(), vers1)
        
        vers2 = (12, 78, 882)
        f.writeComponentVersion(vers2)
        self.assertEqual(f.readComponentVersion(), vers2)
        
        f.close()
        
        # open up again and double check everything was saved to disk
        f = VersionedFile(copyPath, "r+b")
        self.assertEqual(f.readProductVersion(), vers1)
        self.assertEqual(f.readComponentVersion(), vers2)
        self.assertEqual(f.readSignature(), 'RSRC')
        f.close()
        
        # delete the copy
        if os.path.exists(copyPath):
            os.unlink(copyPath)

def suite():
    suite = unittest.makeSuite(VersionedFileUnitTest)
    return suite
    
if __name__ == "__main__":
    unittest.TextTestRunner(verbosity=2).run(suite())
