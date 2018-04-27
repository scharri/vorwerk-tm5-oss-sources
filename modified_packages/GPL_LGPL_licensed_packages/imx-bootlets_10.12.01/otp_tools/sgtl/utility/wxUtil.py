#-----------------------------------------------------------------------------
# Name:        Util.py
# Purpose:     Utility functions for RPC
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
import wx

import struct as _struct
import time as _time
import datetime as _datetime
import sys as _sys
import os as _os
#import types, string

binPath = _os.path.abspath("..\\..\\")           # point to ./bin
_sys.path.append(binPath)                        # import sgtl library
import sgtl.rpc as rpc                          # rpc library
from sgtl.rpc.Socket import *                   # rpc socket library

#//////////////////////////////////////////////////////////////////////////////
# Constants
#//////////////////////////////////////////////////////////////////////////////
FAIL    = 1
SUCCESS = 0

# modes of verbosity
QUIET_MODE   = 2            # print error only
NORMAL_MODE  = 1            # print warn and error
VERBOSE_MODE = 0            # print info, wanr, and error

#Levels of log
ERROR = 2
WARN  = 1
INFO  = 0

# port numbers
LOGPORT  = 50001             # Log Agent
CNTLPORT = 50003             # RPC Control Stream
BULKPORT = 50004             # RPC Bulk Stream

# common chip address
HW_DIGCTL_CHIPID    = 0x8001C310

# RPC command types
BASE_RPC_CMD        = 0x40B200
RPC_VER             = BASE_RPC_CMD
RPC_READ            = BASE_RPC_CMD+1
RPC_WRITE           = BASE_RPC_CMD+2


#//////////////////////////////////////////////////////////////////////////////
#//////////////////////////////////////////////////////////////////////////////
class wxRpcUtil(object):

    def __init__(self):
        self.logFilename    = None
        self.LogHandle      = None
        self.VerbosityLevel = WARN
        self.systemStart    = _datetime.datetime.now()
        self.stdRpcCmdList  = self.buildStdCmdList()
        return

    #//////////////////////////////////////////////////////////////////////////////
    # Print to Screen and Log File
    #//////////////////////////////////////////////////////////////////////////////
    def log(self, lvl, msg):
        """ msg is a string"""
        if lvl >= self.VerbosityLevel:
            print(msg)
            if self.LogHandle == None:
                self.logCreate(self.logFilename)
            if self.LogHandle != None:
                try:
                    self.LogHandle.write(msg+"\n")
                except:
                    pass
        return

    #//////////////////////////////////////////////////////////////////////////////
    def logCreate(self, logFilename=None):
        """Open file"""
        if logFilename != None:
            self.LogHandle   = open(logFilename, 'w')

    #//////////////////////////////////////////////////////////////////////////////
    # Set Verbosity level
    #//////////////////////////////////////////////////////////////////////////////
    def setVerbosityLevel(self, level):
        """eVerboseLevel = QUIET_MODE, NORMAL_MODE, VERBOSE_MODE"""
        self.VerbosityLevel = level

    #//////////////////////////////////////////////////////////////////////////////
    # Get Verbosity level
    #//////////////////////////////////////////////////////////////////////////////
    def getVerbosityLevel(self):
        """eVerboseLevel = QUIET_MODE, NORMAL_MODE, VERBOSE_MODE"""
        self.VerbosityLevel = level

    #//////////////////////////////////////////////////////////////////////////////
    # Print title to log file
    #//////////////////////////////////////////////////////////////////////////////
    def title(self,msg):
        """Print title to screen and log"""
        str = "- %s -" % msg
        leng = len(str)
        print "-"*(leng)
        self.log(WARN, str)
        print "-"*(leng)
        return

    #//////////////////////////////////////////////////////////////////////////////
    # Print Test Result (formatted output)
    #//////////////////////////////////////////////////////////////////////////////
    def printStatus(self, msg, eStatus):
        """eStatus = 0=>SUCCESS 1+=>FAIL"""
        if eStatus == PASS:
            str = "%s: Passed" % msg
        else:
            str = "%s: Failed" % msg
        self.log(INFO, str)
        return

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def dialogBox(self, title, msg, shutdown=False):
        """print a dialog box, then shutdown (if requested)"""
        dlg = wx.MessageDialog(self, msg, title, wx.OK | wx.ICON_INFORMATION)
        try:
            dlg.ShowModal()
        finally:
            dlg.Destroy()
        if shutdown:
            self.shutdown(FAIL)
        return

    #//////////////////////////////////////////////////////////////////////////////
    # wxPython is not yet running, use TKL instead
    #//////////////////////////////////////////////////////////////////////////////
    def msgBox(self, title, msg, shutdown=False):
        """print a dialog box, then shutdown (if requested)"""
        #dlg = wx.MessageDialog(self, msg, title, wx.OK | wx.ICON_INFORMATION)
        #try:
        #    dlg.ShowModal()
        #finally:
        #    dlg.Destroy()
        print(msg)
        if shutdown:
            self.shutdown(FAIL)
        return

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def socketConnect(self, name, host, port, timeout):
        """Connects a socket to a stream """
        socket = rpc.Socket.RpcSocket()
        eStatus = socket.open(host, int(port), float(timeout))
        if eStatus == SUCCESS:
            self.log(INFO, "Connect: %s SUCCESS" % name)
        else:
            self.log(ERROR, "Connect: %s FAILED" % name)
            try:
                self.DialogBox( 'Port Connect Failure',
                    'Verify DEMUX.EXE is running and \nthat port %d is configured'%int(port), True)
            except:
                pass
            return None
        return socket

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def socketClose(self, socket):
        """Shuts down socket"""
        try:
            socket.close()
        except:
            pass
        return

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def startTest(self):
        return _datetime.datetime.now()

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def endTest(self, startTime=None, endTime=None ):
        if endTime == None:
            endTime = _datetime.datetime.now()
        if startTime == None:
            startTime = self.systemStart # default to system start time
        delta = endTime - startTime
        minutes = delta.seconds/60
        seconds = delta.seconds - (minutes*60)
        self.log(WARN, "Time = %0d min %0d sec %03.3d mS" % (minutes, seconds, delta.microseconds/1000))
        return

    #//////////////////////////////////////////////////////////////////////////////
    # exit script with error code
    #//////////////////////////////////////////////////////////////////////////////
    def shutdown(self, eStatus, startTime=None):
        """exit with error code"""
        self.auditAll = True
        self.endTest(None, _datetime.datetime.now())
        if eStatus != 0:
            self.log(ERROR, "Exiting with Error.  RC=%d" % eStatus)
        else:
            print "Done"
        if self.LogHandle != None:
            self.LogHandle.close()
        return

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def GetCmdLineOptions(self):
        """return INI options"""
        import getopt
        from optparse import OptionParser

        # Setup Defaults
        usage = """%prog [options]"""

        # Parse args for reals
        parser = OptionParser(usage=usage)

        # Verbosity
        parser.add_option("-v", "--verbose", type="int", dest="verbose", default=1,
            metavar="", help="Verbose Mode [0=quiet; 1=soft(default); 2=loud]")

        # Pull all given options into the options variable
        args = None
        (options, args) = parser.parse_args()

        # Initial option values
        options.HostName = 'localhost'       # Local PC running DEMUX
        options.LogPort  = LOGPORT           # Log Agent
        options.HostPort = CNTLPORT          # RPC Control Stream
        options.BulkPort = BULKPORT          # RPC Bulk Stream
        options.Timeout  = 0.5               # Response timeout of socket (in seconds)
        options.iniFile  = "rpcnet.ini"
        options.logName  = "rpcLog.txt"

        # Here we put the options back into the global namespace
        # (this only works if all the options specify default values)
        return options

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def cmdItem(self, cmdTxt, cmdNum, cmdPlist, cmdHelp=""):
        item = []
        item.append(str(cmdTxt))
        item.append(int(cmdNum))
        item.append(str(cmdPlist))
        item.append(str(cmdHelp))
        return item

    def buildStdCmdList(self):
        """returns a list of RPC standard commands for command text parsing"""
        list = []
        list.append(self.cmdItem("RPCV",RPC_VER,"d"))
        list.append(self.cmdItem("RPCR",RPC_READ,"d"))
        list.append(self.cmdItem("RPCW",RPC_WRITE,"d,d"))
        return list

    #//////////////////////////////////////////////////////////////////////////////
    #//////////////////////////////////////////////////////////////////////////////
    def sendCmd(self, cntlChan, cmd, timeout=2, cmdList=None):
        """Send a text command and parse response"""
        if cmdList == None:
            cmdList = self.stdRpcCmdList

        # convert text to RPC command (cntlChan = self.Cntl)
        status = cntlChan.convertText2Rpc(cmd, cmdList)
        if status != SUCCESS:
            self.log(ERROR, "Error Parsing Command")
            return FAIL, -1

        #send rpc command and wait for response
        #startTime = _datetime.datetime.now()
        status = cntlChan.SendCmd(cntlChan.buffer, timeout)
        #self.endTest(startTime)
        if (status != SUCCESS):
            if cntlChan.header == None:
                self.log(ERROR, "RX->I/O Timeout\r\n")
            else:
                self.log(ERROR, "RX->ERROR [%X]\r\n" % cntlChan.header.errorCode)
            return FAIL, -1
        # process reponse
        data = self.processRsp( cntlChan.header, cntlChan.buffer)
        return SUCCESS, data


    def processRsp(self, header, buffer):
        """Parses a Cntl stream response message """
        data = 0
        if header.errorCode != SUCCESS:
            # error on response side
            msg = rpc.Rpc.GetErrorStr(header)
            self.log(ERROR, "RX->FAIL %s [%X]\r\n" % (msg,header.errorCode))
        else:
            BufferLeng = len(buffer)
            if BufferLeng < header.sizeof:
                self.log(ERROR, "RX->(Short Response)")         # packet too short
            elif BufferLeng == header.sizeof:
                errString = rpc.Rpc.GetErrorStr(header)  # no data, just a reponse
                self.log(INFO, "RX->(%s)" % errString)
            else:
                dataLeng = BufferLeng-header.sizeof
                errString = rpc.Rpc.GetErrorStr(header)
                # create a format that will fit buffer (hdr+data)
                format = rpc.Rpc.buildResponseBuffer(BufferLeng)
                self.log(INFO,"RX->(%s) [" % errString)
                try:
                    tup = _struct.unpack(format, buffer)
                    firstItem=True
                    for idx in range(3,len(tup)):  #use 3 to skip header
                        if firstItem == True:
                            first = False
                        else:
                            self.log(", ")
                        self.log(INFO, "0x%X" % (tup[idx]))
                        data = tup[idx]
                    self.log(INFO, "]")
                except:
                    self.log(ERROR, "Unpack reponse error")
            #self.log("  %d bytes processed\r\n" % (dataLeng))
        return data

#//////////////////////////////////////////////////////////////////////////////
# Start main function when run directly
#//////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":

    self = wxRpcUtil()

    testTimer   = False
    testDialog  = False
    testLog     = False
    testSocket  = True

    self.title("wxPython Utils")

    if testTimer == True:
        start = self.startTest()
        _time.sleep(1.0)  # floating point value
        self.endTest(start)

    if testDialog == True:
        # requires a window to be created first...
        self.dialogBox("title","message")

    if testLog == True:
        # test logging
        self.setVerbosityLevel(QUIET_MODE)
        self.printStatus("QMODE: printStat failed",OK)
        self.log("QMODE: Audit failed")
        self.setVerbosityLevel(NORMAL_MODE)
        self.printStatus("NMODE: printStat pass",OK)
        self.log("NMODE: Audit pass")
        self.setVerbosityLevel(VERBOSE_MODE)
        self.printStatus("VMODE: printStat pass",OK)
        self.log("VMODE: Audit pass")
        self.auditAll = True
        self.printStatus("QMODE: printStat pass",OK)

    if testSocket == True:
        #test basic command sending
        self.Cntlsocket = self.socketConnect( "Cntl", "localhost", CNTLPORT, 1)
        if self.Cntlsocket != None:
            # establish RPC command to socket
            self.Cntl = rpc.Cmd.CmdChan(self.Cntlsocket)

            status, data = self.sendCmd(self.Cntl, "RPCV")
            self.log(WARN, "Status=%X" % (status))

            status, data = self.sendCmd(self.Cntl, "RPCR %s" %(HW_DIGCTL_CHIPID))
            self.log(WARN, "Chip=%X" % (data>>16))
            self.log(WARN, "Version=%X" % (data&0xFF))

            #close channel
            self.socketClose(self.Cntl.socket)

    #_time.sleep(1.0)  # floating point value
    self.shutdown(SUCCESS)
