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

import sys, os, time
import unittest
import subprocess

class procmon():
    def __init__(self):
        self.enabled   = False
        self.run       = False
        self.exepath   = "C:\\Program Files\\ProcMon"
        self.logpath   = "C:\\temp"
        self.debugMode = False
        if os.path.isfile(os.path.join(self.exepath,"enabled.txt")):
            self.enabled = True
            self.run = False
            self.exe = os.path.join(self.exepath,"procmon.exe")
            self.log = os.path.join(self.logpath,"trace.pml")
            self.opt = "/minimized /quiet /backingfile c:\\temp\\trace.pml"
            self.pid = None
        return

    def ClearOutput(self):
        """ Clear out old logs """
        if self.enabled == False:
            return
        savFile=self.log+".sav"
        if os.path.isfile(savFile):
            print("Removing %s..." % savFile)
            os.remove(savFile)
        if os.path.isfile(self.log):
            if self.debugMode == True:
                print("Removing %s..." % self.log)
            os.remove(self.log)
        #clear out extra files
        x = 1
        while x in range(1,50):
            file = "%s\\trace-%d.pml" % (self.logpath,x)
            if os.path.isfile(file):
                if self.debugMode == True:
                    print("Removing %s..." % file)
                os.remove(file)
            x += 1
        return

    def Start(self):
        """ Start monitoring all process information """
        if self.enabled == False:
            return 0
        if self.run == False:
            self.ClearOutput()
            self.run = True
            print("Starting ProcMon...")
            try:
                self.pid = subprocess.Popen([self.exe, self.opt]).pid
            except:
                self.pid = None
        return 0
    
    def Stop(self):
        """ Stop monitoring process information. """
        retcode = 0
        if self.run == True:
            print("Stopping ProcMon...")
            # Wait for process to end before continuing
            try:
                retcode = subprocess.call("\"" + self.exe + "\" /terminate", shell=True)
                if retcode < 0:
                    print >>sys.stderr, "ProcMon: Child was terminated by signal", -retcode
                else:
                    print >>sys.stderr, "ProcMon: Child returned", retcode
                if retcode == 0:
                    self.run = False
            except OSError, e:
                print >>sys.stderr, "ProcMon: Execution failed:", e
        return retcode

    def Save(self):
        """ Move log files to a safe location for post analysis """
        if self.run == True:
            self.Stop()
            print >>sys.stderr, "ProcMon: Saving log files..."
            # rename primary log file.  There may be up to 30 other logs produced.
            if os.path.isfile(self.log):
                savFile=self.log+".sav"
                if self.debugMode == True:
                    print("Renaming %s -> %s ..." % (self.log,savFile))
                os.rename(self.log,savFile)
        return
    
    def DebugMode(self,flag=True):
        self.debugMode = flag
        return
    
# Run unit tests when this source file is executed directly from the command
# line.
if __name__ == "__main__":
    mon = procmon()
    mon.DebugMode()
    
    # Inital test
    print("Capturing process data...")
    mon.Start()
    time.sleep(10.0)
    mon.Save()
    mon.Stop()
    
    # see if cleanup works.
    print("Testing cleanup...")
    mon.Start()
    mon.Stop()
    
    print "Test Complete"
