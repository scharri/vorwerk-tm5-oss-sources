#-----------------------------------------------------------------------------
# Name:        wxCheck.py
# Purpose:     Verify valid version of wxPython
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

_SUCCESS = 0
_FAIL = 1

from Tkinter import *
from tkMessageBox import *
from tkFileDialog import askopenfilename      
import os as _os
import re

#-------------------------------------------------
# This will only work for internal development.
# Customers will need to download the image.
#
def wxVerCheck(self,VerbosityLevel=0):
    """VerbosityLevel (0=OFF) (!0=ON)"""
    try:
        import wx
        minVer = [2,8,9]
        ver = wx.version()
        p = re.compile(r'[.]+')
        m = p.split( ver )
        n = [int(m[0]),int(m[1]),int(m[2])]
        if n < minVer:
            generate_except
    except:
        # use TK for questions        
        response = askquestion('Error', 'wxPython upgrade required. \nDo you want to install now?')
        if response == "yes":
            folder = _os.path.abspath("..\..\..\utilities\python")
            cmd = "%s\checkAndinstall.py --wxPython --path=%s" %(folder,folder)
            stat = _os.system(cmd)
            response = showwarning('Restart', "You must now restart script")
        else:
            print" wxPython must be installed/upgraded in order to use run this script"
            print" See www.wxPython.org"
        print
        return _FAIL
            
    return _SUCCESS

#//////////////////////////////////////////////////////////////////////////////
#//////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':
    wxVerCheck(0)
    print"\n...Done..."

