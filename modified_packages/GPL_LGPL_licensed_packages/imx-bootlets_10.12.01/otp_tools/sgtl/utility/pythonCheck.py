#-----------------------------------------------------------------------------
# Name:        pythonCheck.py
# Purpose:     Common getch for Unix and DOS
#
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
#-----------------------------------------------------------------------------
#!/bin/env python

# API Version
import struct as _struct
import time as _time
import sys as _sys

_SUCCESS = 0
_FAIL = 1

#//////////////////////////////////////////////////////////////////////////////
# Python Support Check
#   MaxMajor, MaxMinor, MaxInc = 4.2.999
#   MinMajor, MinMinor, MinInc = 4.2.0
#//////////////////////////////////////////////////////////////////////////////
def pythonVerCheck(VerbosityLevel=0, Now=None):
    """
    VerbosityLevel (0=OFF) (!0=ON)

    Checks the version number of the current python installation and against
    a minimum and maximum provided by the caller.  Version numbers should be
    provided in tuple form as follows: (Major, Minor, Micro)
    e.g. version 2.4.2 should be passed in as the tuple (2, 4, 2)

    Returns:
    * 0 if version requirements are met
    * An error code != 0 if they are not
    """
    MinPythonVer = ( 2, 4, 0 )
    RecPythonVer = ( 2, 5, 0 )
    MaxPythonVer = ( 2, 6, 99 )
    eStatus = _SUCCESS
    if Now == None:
        Now = _sys.version_info[0:3]

    # Right now we only look for minimum requirements (ignore Incremental)
    if Now < MinPythonVer:
        print ("Current Python version %d.%d.%d" % Now)
        print ("Script requires at least Python version %d.%d.%d" % MinPythonVer)
        print (" ")
        print ("It is recommended that you install python %d.%d" % (RecPythonVer[0],RecPythonVer[1]))
        eStatus = _FAIL

    if Now > MaxPythonVer:
        print ("Current Python version %d.%d.%d" % Now)
        print ("Support has only been tested to version %d.%d" % (MaxPythonVer[0],MaxPythonVer[1]))
        print (" ")
        print("It is recommended that you install python %d.%d" % (RecPythonVer[0],RecPythonVer[1]))
        eStatus = _FAIL

    return(eStatus)


#//////////////////////////////////////////////////////////////////////////////
#//////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':
    status = pythonVerCheck(1)
    print"status = %d\n" % status
    
    status = pythonVerCheck(1,( 2, 3, 0 ))
    print"status = %d\n" % status
    
    status = pythonVerCheck(1,( 2, 7, 0 ))
    print"status = %d\n" % status
    

