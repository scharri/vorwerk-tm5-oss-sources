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

import sys, os
import externaltool
import unittest
import controller
import re

__all__ = ["BUILD_INCREMENTAL", "BUILD_ALL", "BUILD_CLEAN", "ProjectBuildException", "MultiBuildOperation", "ReadGhsToolsDir", "buildMulti", "suite"]

# Build modes
BUILD_INCREMENTAL = 0
BUILD_ALL = 1
BUILD_CLEAN = 2

buildModeStringMap = { BUILD_INCREMENTAL : "incremental", BUILD_ALL : "all", BUILD_CLEAN : "clean" }

# determine system root path
if sys.platform == "win32":
    SYSTEM_ROOT_PATH = "C:\\"
elif sys.platform == "cygwin":
    SYSTEM_ROOT_PATH = "/cygdrive/c"
else:
    SYSTEM_ROOT_PATH = "/"

# build GHS tools path
#GHS_BASE_PATH = os.environ['GHS_TOOLS_DIR']
GHS_BUILD_TOOL = "gbuild"

# An exception class that contains information about which project failed to
# build and why.
class ProjectBuildException:
    def __init__(self, projectPath, buildMode, commandLine, resultCode):
        self.projectPath = projectPath
        self.buildMode = buildMode
        self.commandLine = commandLine
        self.resultCode = resultCode

    # format a nice user-facing string describing the error
    def __str__(self):
        if buildModeStringMap.has_key(self.buildMode):
            buildModeString = buildModeStringMap[self.buildMode]
        else:
            buildModeString = "unknown"
        return "Building %s (%s) failed with error %d" % (self.projectPath, buildModeString, self.resultCode)

    def __repr__(self):
        return str(self)

# The operation class that calls gbuild.
class MultiBuildOperation(externaltool.ExternalToolOperation):
    def __init__(self):
        externaltool.ExternalToolOperation.__init__(self)
        self._projectPath = None
        self._subprojectPath = None
        self._mode = BUILD_INCREMENTAL
        self._parallels = 4
        self._ignoreErrors = False
        self._showInternalCmds = False
        
    def setTopLevelProjectPath(self, path):
        self._projectPath = path
        return self
    
    def setSubprojectPath(self, path):
        self._subprojectPath = path
        return self
    
    def setMode(self, mode):
        self._mode = mode
        return self
    
    def setParallelProcesses(self, count):
        self._parallels = count
        
    def setIgnoreErrors(self):
        self._ignoreErrors = True
        
    def setShowInternalCmds(self):
        self._showInternalCmds = True
             
    def runInternal(self, context):
        # Learn the desired location of the GHS tools.
        try:
            theBasePath = ReadGhsToolsDir()
            GHS_BASE_PATH = theBasePath
        except:
            raise Exception("MultiBuildOperation could not set GHS_BASE_PATH" )        

        # Tell the sgtl.build infrastructure where to find the GHS tools.
        self.setCommandPath(os.path.join(GHS_BASE_PATH, GHS_BUILD_TOOL))
        # N.B.:  Hereafter, op.getCommandPath() will return this path.
        # Also, sgtl.build will look for the GHS tools there.

        # Must have a top level project set.
        if not self._projectPath:
            raise Exception("no project path was specified")
        
        # Build arguments.
        if self._mode == BUILD_ALL:
            self.addArgument("-cleanfirst")
            self.addArgument("-all")
        elif self._mode == BUILD_CLEAN:
            self.addArgument("-clean")
        
        if self._ignoreErrors:
            self.addArgument("-ignore")
        if self._showInternalCmds:
            self.addArgument("-commands")
            self.addArgument("-nested_commands")
        self.addArgument("-parallel=%d" % self._parallels)
        self.addArgument("-top")
        self.addArgument(self.joinAndNormalizePath(self._projectPath))
        if self._subprojectPath:
            self.addArgument(self.joinAndNormalizePath(self._subprojectPath))
        
        returncode = self.copyOutputToLog(self.executeCommand())

        # Raise exception if the build failed.
        if returncode != 0:
            raise ProjectBuildException(self._projectPath, self._mode, '', returncode)


def ReadGhsToolsDir():
    """
    This is a utility function that knows how to read
    a file that points to the GHS tools directory.

    The aforementioned file provides a single point of information by which
    to designate the GHS tools directory.  This capability allows
    that GHS tools directory to be changed by modifying the single point of
    information.
    """

    foundDir = None

    # This is the name of the batch file that designates the directory
    # containing the GHS tools.
    batFileName     = os.path.join( controller.buildContext.baseDirectory(), 'bin', 'set_ghs_tools_dir.bat' )

    try:
        batFile = file(batFileName, 'r')
    except:
        print Exception("ReadGhsToolsDir could not find " + batFileName)
        raise

    # Search for the name of the GHS tools directory in that batch file.
    reGHS_TOOLS_DIR = re.compile("GHS_TOOLS_DIR=(.+)")
    ghsDirs = []
    for oneLine in batFile:
        mW = reGHS_TOOLS_DIR.search(oneLine)
        if mW:
            # Found the name of the directory.
            ghsDirs.append(mW.group(1))

    batFile.close()
    
    # Verify which ghs directory exists on the machine currently executing this script
    for dir in ghsDirs:
        if os.path.exists(dir):
            foundDir = dir

    if not foundDir:
        print Exception("ReadGhsToolsDir could not find GHS_TOOLS_DIR in " + batFileName)
        raise

    return foundDir


def buildMulti(topLevelProject, subproject=None, buildMode=BUILD_INCREMENTAL, sourceRoot=None):
    """
    Convenience function to create a Multi build-operation.

    topLevelProject:    The name of a GHS gpj project.
    subproject:         A subsidiary gpj project.
    buildMode:          e.g. BUILD_INCREMENTAL.
    sourceRoot:         A convenience parameter to allow buildMulti to
                        initialize the sgtl.build.controller with the name
                        of the root directory for the SDK.
                        Nominally 'SOCFirmware'.

    NOTE:   Before calling buildMulti, you may want to call 
            setBaseDirectoryToSourceRoot() which is in sgtl.build.
            Alternatively, the buildMulti sourceRoot parameter can be set.
    """

    op = MultiBuildOperation()

    # If we were given a sourceRoot parameter, then use it.
    if sourceRoot:
        controller.buildContext.setBaseDirectoryToSourceRoot( sourceRoot )

    op.setContext(controller.buildContext)
    op.setTopLevelProjectPath(topLevelProject)
    if subproject:
        op.setSubprojectPath(subproject)
    op.setMode(buildMode)

    return op


# Unit test for this module.
class MultiBuildOperationUnitTest(unittest.TestCase):
    def setUp(self):
        self.projectPath = os.path.join('..', '..', '..', 'projects', 'utilities', 'boot_manager', 'boot_manager_top.gpj')
        
    def test_build(self):
        op = MultiBuildOperation()
        op.setMode(BUILD_ALL)
        op.setTopLevelProjectPath(self.projectPath)
        if sys.platform == 'win32':
            op.run()
    
    def test_quick_build(self):
        op = buildMulti(self.projectPath, buildMode=BUILD_ALL)
        if sys.platform == 'win32':
            op.run()

# Returns a TestSuite object containing all unit tests for this module.
def suite():
    suite = unittest.makeSuite(MultiBuildOperationUnitTest)
    return suite

# run unit tests is called directly
if __name__ == "__main__":
    result = unittest.TextTestRunner(verbosity=2).run(suite())
    if len(result.errors) or len(result.failures):
        sys.exit(1)
