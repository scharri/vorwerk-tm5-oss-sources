#!/bin/env python

#-----------------------------------------------------------------------------
# Name:        rpc_vmi_stats.py
# Purpose:     Installs PYSCO if available.  PSYCO will speed up a python script
#              from 2 to 10 times.
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

#-------------------------------------------------------------------------------
# We'd like to install the psyco optimizer because it greatly improves
# performance. However, if we're being debugged, psyco will screw things up.
# Begin by assuming we are not being debugged.
#-------------------------------------------------------------------------------
_weAreBeingDebugged = False
_debug = True

#-------------------------------------------------------------------------------
# Check if we're being debugged by Komodo. The following test is documented at:
#   http://community.activestate.com/forum-topic/how-can-python-know-that
#-------------------------------------------------------------------------------
import logging
if logging.Logger.manager.loggerDict.has_key('dbgp'):
    _weAreBeingDebugged = True

#-------------------------------------------------------------------------------
# Check if we're being debugged by PyDev in Eclipse.
#-------------------------------------------------------------------------------
import sys
if ("pydevd" in sys.modules.keys()):
    _weAreBeingDebugged = True
#-------------------------------------------------------------------------------
# Check if we're being debugged by IDLE.
#-------------------------------------------------------------------------------
if ("idlelib" in sys.modules.keys()):
    _weAreBeingDebugged = True
#-------------------------------------------------------------------------------
# Check if we're being debugged by winpdb.
#-------------------------------------------------------------------------------
if ("rpdb2" in sys.modules.keys()):
    _weAreBeingDebugged = True

#-------------------------------------------------------------------------------
# If we're not being debugged, attempt to import psyco. If it's not actually
# installed on this machine, we'll get an exception. But we're friendly - we
# catch the exception and keep going.
#-------------------------------------------------------------------------------
if (_weAreBeingDebugged):
    print ("Psyco is disabled during debug")
else:
    try:
        import psyco
        psyco.full()
        if _debug:
            print 'Psyco installed'
    except:
        if _debug:
            print 'Psyco not installed'
        pass
