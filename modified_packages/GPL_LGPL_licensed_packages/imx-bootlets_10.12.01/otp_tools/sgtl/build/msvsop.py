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
import externaltool
import unittest
import controller
import multibuildop

__all__ = ["MSVSOperation", "buildVS", "suite"]

# determine system root path
if sys.platform == "win32":
    SYSTEM_ROOT_PATH = "C:\\"
elif sys.platform == "cygwin":
    SYSTEM_ROOT_PATH = "/cygdrive/c"
else:
    SYSTEM_ROOT_PATH = "/"

# Operation subclass to build a MS Visual Studio project.
#
# XXX use a temporary file for the build output and copy its contents out
#     through our log handler object
class MSVSOperation(externaltool.ExternalToolOperation):
    def __init__(self, solution=None, config=None, rebuildIt=False, logFile=None, bClean=False ):
        externaltool.ExternalToolOperation.__init__(self)
        self._solution = solution
        self._config = config
        self._rebuild = rebuildIt
        self._bClean = bClean
        self._log = logFile
        
    def setSolutionFile(self, path):
        self._solution = path
    
    def setConfiguration(self, name):
        self._config = name
    
    def setRebuild(self, rebuildIt):
        self._rebuild = rebuildIt
        
    def setClean(self, bClean):
        self._bClean = bClean
        
    def setBuildLogFile(self, path):
        self._log = path
    
    def runInternal(self, context):
        if not self._solution:
            raise Exception("No solution path provided")
        if not self._config:
            raise Exception("No solution configuration provided")
        
        # build GHS tools path
        VS_VERSION = "Microsoft Visual Studio .NET 2003"
        VS_BASE_PATH = os.path.join(SYSTEM_ROOT_PATH, "Program Files", VS_VERSION)
        VS_BUILD_TOOL = "devenv.exe"
        VARS_BAT_FILE_NAME = r"vsvars32.bat"
        
        #
        # Try to find the Visual Studio tools directory in an environment variable.
        # If found, then use that directory instead of the one hardcoded in this script.
        #
        temp_vs_comn_tools_path = os.getenv('VS71COMNTOOLS')
        if temp_vs_comn_tools_path:
            VS_COMN_TOOLS_PATH = temp_vs_comn_tools_path
            VARS_BAT_FILE_PATH = os.path.join( VS_COMN_TOOLS_PATH, VARS_BAT_FILE_NAME )
        else:
            VS_COMN_TOOLS_PATH = os.path.join(VS_BASE_PATH, r"Common7\Tools")
            VARS_BAT_FILE_PATH = os.path.join( VS_COMN_TOOLS_PATH, VARS_BAT_FILE_NAME )

        varsBatPath = VARS_BAT_FILE_PATH
        solutionPath = self.joinAndNormalizePath(self._solution)
        
        self.setCommandPath(varsBatPath)
        self.addArgument('&&')
        self.addArgument(VS_BUILD_TOOL)
        self.addArgument(solutionPath)
        if self._rebuild:
            self.addArgument("/rebuild")
        else:
            self.addArgument("/build")
        if self._config:
            self.addArgument(self._config)
        if self._bClean:
            self.addArgument("/clean")
        if self._log:
            logPath = self.joinAndNormalizePath(self._log, os.getcwd())
            
            # Delete the log file if it already exists.
            if os.path.exists(logPath):
                os.unlink(logPath)
            
            self.addArgument("/out")
            self.addArgument(logPath)
        
        returncode = self.copyOutputToLog(self.executeCommand())

        # Raise exception if the build failed.
        if returncode != 0:
            raise multibuildop.ProjectBuildException(solutionPath, 0, '', returncode)

def buildVS(solution, config=None, rebuildIt=False, logFile=None):
    op = MSVSOperation(solution)
    op.setContext(controller.buildContext)
    if config:
        op.setConfiguration(config)
    op.setRebuild(rebuildIt)
    op.setBuildLogFile(logFile)
    return op

# Return a suite containing all the test cases for this module.
def suite():
    # Unit test for move operation.
    class MSVSOperationUnitTest(unittest.TestCase):
        def setUp(self):
            self.projectPath = os.path.join('..', '..', '..', 'utilities', 'cfimager', 'cfimager.sln')
            
        def test_build(self):
            op = MSVSOperation()
            op.setSolutionFile(self.projectPath)
            op.setConfiguration("Debug")
            op.setRebuild(True)
            op.setBuildLogFile("build.log")
            if sys.platform == 'win32':
                op.run()
        
        def test_quick_build(self):
            op = buildVS(self.projectPath, "Debug", True, "build.log")
            if sys.platform == 'win32':
                op.run()
    
    studioSuite = unittest.makeSuite(MSVSOperationUnitTest)
    suite = unittest.TestSuite((studioSuite))
    return suite

# Execute this module's unit tests when called from the command line.
if __name__ == "__main__":
    result = unittest.TextTestRunner(verbosity=2).run(suite())
    if len(result.errors) or len(result.failures):
        sys.exit(1)
