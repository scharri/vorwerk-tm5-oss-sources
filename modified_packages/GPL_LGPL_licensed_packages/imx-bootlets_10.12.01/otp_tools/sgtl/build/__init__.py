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
import operations
import errorhandler
import loghandler
import configop
import multibuildop
import doxyop
import versionop
import controller
import parseprojectstobuild

# Import quick operation constructors into this module namespace.
from externaltool import tool
from multibuildop import buildMulti
from multibuildop import BUILD_INCREMENTAL, BUILD_ALL, BUILD_CLEAN
from multibuildop import ReadGhsToolsDir
from configop import config
from copyop import copy, move, delete, rename
from msvsop import buildVS
from doxyop import doxygen
from versionop import setVersion

# create our single controller class instance
controller.buildContext = controller.BuildController()
controller.buildContext.setBaseDirectory('.')

logHandler = loghandler.DefaultLogHandler()
logHandler.addOutputStream(sys.stderr)
controller.buildContext.setLogHandler(logHandler)

errorHandler = errorhandler.DefaultErrorHandler()
errorHandler.setLogHandler(logHandler)
errorHandler.setExitOnError(True)
controller.buildContext.setErrorHandler(errorHandler)

def findSourceTreeRoot():
    path = os.getcwd()

    while True:
        path, thisDir = os.path.split(path)
        dirList = os.listdir(path)
        if len(thisDir) == 0:
            return ''
        elif 'bin' in dirList:
            if len(path) != 0:
                return path
            else:
                return ''

def resetOutputs():
    controller.buildContext.logHandler().resetOutputs()

def setBaseDirectoryToSourceRoot():
    setBaseDirectory(findSourceTreeRoot())

def setBaseDirectory(path):
    controller.buildContext.setBaseDirectory(path)

def setLogHandler(handler):
    controller.buildContext.setLogHandler(handler)

def setErrorHandler(handler):
    controller.buildContext.setErrorHandler(handler)

def setLogFile(filePath, mode="w"):
    controller.buildContext.logHandler().addOutputFile(filePath, mode)

def closeLogFile(filePath):
    controller.buildContext.logHandler().closeOutputFile(filePath)

def log(message):
    controller.buildContext.logHandler().log(message)

def logThisFile( fileName, mode='r' ):
    controller.buildContext.logHandler().logThisFile( fileName, mode )


