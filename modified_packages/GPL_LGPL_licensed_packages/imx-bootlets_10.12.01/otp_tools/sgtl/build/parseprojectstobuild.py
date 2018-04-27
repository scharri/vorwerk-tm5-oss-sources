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

## \ingroup sgtl
# @file parseprojectstobuild.py
#
# This file is used to parse the projects_to_build.txt file in the SOCFirmware/bin
# directory. The file parser builds up a list of files that need to be copied,
# built or configured. In addition, the parser can also execute a series of
# commands on the command line.  This file is primarily used by the build_new_sdk.py
# or (buildSDK.py in the future) at the beginning to configure gpjs and generate
# a list of gpjs that need to be built.
#
# Text File Syntax:
#
# **All paths need to have forward slashes instead of back slashes
#
# **GPJ files are relative to SOCFirmware directory since this is the originating file.
#   Other include text files should have the project file paths relative to where the text file is located.
#
# Copying a file or directory 
#   $$copy <path to source> <path to destination> (<build goal> , <chip select> )
#
# Including a link to another text file:
#   $$include <full or relative path to text file> (<build goal> , <chip select> )
#
# Listing a GPJ file to build:
#   <full or relative path to gpj file> (<build goal> , <chip select> )
#
# Configure and build a GPJ file:
#   <full or relative path to output gpj> , <full or relative path to tgpj> , <config or properties> , <full or relative path to cfg file> (<build goal> , <chip select> )
#
# Only configure a gpj
#   $$configure <full or relative path to output gpj> , <full or relative path to tgpj> , <config or properties> , <full or relative path to cfg file> (<build goal> , <chip select> )
#
# Build Goal Options: (make sure they are lowercase)
#   all_goals (default when none selected)
#   dev
#   merge
#   qa
#   extract
#   customer
#   bass
#
# Chip Select Options: (make sure they are lower case)
#   36xx  (default when none selected)
#   37xx
#   all_chips
#
# Running an external program or script
#   $$run <command line> (<build goal> , <chip select> )
#  
#   Use '##BIN##' as a constant to represent the full path to the bin directory.


import sys
import os
import re
import subprocess
import tempfile
import shutil
import unittest
import controller
import operations
import types
import externaltool
import __init__

from configop import config


__all__ = ["ParseProjectsToBuildTxt"]

class ParseProjectsToBuildTxt(operations.OperationBase,
                                    externaltool.ExternalToolOperation):
    """The ParseProjectsToBuildTxt class handles all the parsing of external
    command files.
    
    """
    
    
    def __init__(self, stopOnError=False, buildGoal=None, buildMode=None,
                 chipSelect=None, configureOnly=False, commandFile=None,
                 buildErrors=[], logFileName=None, productVersion="5.0.0",
                 componentVersion="5.0.0"):
        
        self._commandFile = commandFile
        
        self._buildGoal = buildGoal
        self._buildMode = buildMode
        self._stopOnError = stopOnError
        self._buildErrors = buildErrors
        self._chipSelect = chipSelect
        self._configureOnly = configureOnly
        self._productVersion = productVersion
        self._componentVersion = componentVersion
        
        self._chipList = ["37xx", "377x", "378x", "all"]
        
        
        self._binDir = os.getcwd()
        self._rootDir = __init__.findSourceTreeRoot()
        
        self._debugMode = False
        self._verbose = False
        
        #Initialize item lists
        self.buildCfgList = []
        self.configureOnlyList = []
        self.notOnBuildCfgList = []
        self.copyFileDict = {}
        self.cleanUpFilesList = []
        self.runCommandDict = {}
        
        self.buildErrors = []
        
        self.setContext(controller.buildContext)
        
        #Determine whether to reconfigure all gpjs or only just update the ones
        #that changed
        if self._buildMode == "BUILD_ALL":
            self._configUpdateMode = "yes"
        else:
            self._configUpdateMode = "update"
            
        if self._buildGoal == "customer" or self._buildGoal == "customer_drm"\
                            or self._buildGoal == "customer_audible":
            self._ignoreMissingTxtFile = True
        else:
            self._ignoreMissingTxtFile = False
        
        #Initialize regular expressions used to parse out commands
        self._re_IncludeFile = re.compile('''^[ ]*[$][$]include\s+.+[.]txt\s*''')
        self._re_ProjFile = re.compile('''^[ ]*[\\/A-Za-z0-9_.]+[.]gpj\s*''')
        self._re_CopyFile = re.compile('''^[ ]*[$][$]copy\s*.+\s+.+''')
        self._re_RunCommand = re.compile('''^[ ]*[$][$]run\s*.+''')
        self._re_SetConstant = re.compile('''^[ ]*[$][$]set\s*.+''')
        self._re_Configure = re.compile('''^[ ]*[$][$]configure\s*.+''')
        self._re_Player = re.compile('''^[ ]*[$][$]player\s*.+''')
        self._re_MultiLine = re.compile('^[ ]*[\[][^]]+[\]]', re.MULTILINE)
        self._re_Version = re.compile('''^[ ]*[$][$]version\s*.+''')
        
        #Gpj Template Paths (releative to bin directory)
        self._updaterTemplatePath = os.path.abspath("../application/framework/sdk_os_media_player/sdk_os_media_updater.tgpj")
        self._hostlinkTemplatePath = os.path.abspath("../application/framework/sdk_os_media_player/hostlink/sdk_os_media_hostlink.tgpj")
        self._playerTemplatePath = os.path.abspath("../application/framework/sdk_os_media_player/sdk_os_media_player_all.tgpj")
    
    
    def _openCommandFile(self):
        """The command file is opened and the contents are extracted into a
        string. 
        
        """
        
        #Open projects_to_build file
        try:
            openFile = open(self._commandFile, 'r')
            #All the lines in the text file are dumped into a list
            commandFileText = openFile.read()
            
            openFile.close()
        except Exception, e:
            commandFileText = ""
            
            self.buildErrors.append(self._commandFile + " was not found.\nDescription: " + str(e) + "\n\n")
            
            
        return commandFileText
    
    
    def _convertMultiLineCmd(self, cmdFile):
        """This multiline syntax will only work only for lines where gpjs are
        listed to be configured and built in build_new_sdk.py.
        
        Format Example:
        [
        <output gpj path> ,
        <tgpj path> ,
        <build options> ,
        <cfg path>
        (<build goal> , <chip selection>)
        ]
        
        This functions reassembles the multiple lines into one single line
        that can be read by the command parser.  The contents of the line
        needs to be enclosed by brackets.
        
        The text file is usually parsed for multi line gpjs listing and creates
        one line versions of the gpj listings.  Then the contents on the text
        file is sent to the command file parser.
        """
        
        commandFileText = cmdFile
        
        multiLineResults = self._re_MultiLine.findall(cmdFile)
        
        for result in multiLineResults:
            
            #Remove brackets and newline characters
            convertedResult = result
            convertedResult = convertedResult.replace('\n', ' ')
            convertedResult = convertedResult.replace('[', '')
            convertedResult = convertedResult.replace(']', '')
    
            commandFileText = commandFileText.replace(result, convertedResult)
            
        return commandFileText.split('\n')
            
         
    def _searchCommandFile(self, cmdFileText, currentDir):
        """Function used to parse the command file contents for gpjs, copy, and
        run commands.  The command file text is passed in as a list.  Each line
        is checked to see if the build goal and chip selection matches what
        the user selected as options in build_new_sdk.py.
        
        """
        
        #Search through list for include text file statements and project files
        for line in cmdFileText:
            #Variable keeps track if line is suppose to be parsed
            parseLine = False
            
            #Determine if line needs to be parsed
            line, parseLine = self._checkBuildOptions(line)
            
            if parseLine:
                
                if self._re_IncludeFile.match(line):
                    
                    self._parseInclude(line, currentDir)
            
                #Search line for gpj project files
                elif self._re_ProjFile.match(line) and len(line.split(',')) == 1:
                    
                    self._parseGpj(line)
                elif self._re_CopyFile.match(line):
                    
                    self._parseCopy(line)
                    
                elif self._re_RunCommand.match(line):
                    
                    self._parseRun(line)
                    
                elif self._re_Configure.match(line):
                    
                    self._parseConfigGpj(line, True)
                    
                elif self._re_Player.match(line):
                    
                    self._parseProject(line)
                    
                elif self._re_Version.match(line):
                    
                    #Take out $$version from configure line
                    line = line.replace("$$version", "")
                    
                    if (len(line.split(',')) == 4 or len(line.split(',')) == 3) and not '#' in line:
                        self._parseConfigGpj(line, version=True)
                    else:
                        self.notOnBuildCfgList.append(line.strip())
                    
                #Search line for gpj files that need to be configured
                #Format: <relative or full path to output gpj>,<relative or full path to tgpj>,<config values>,<relative or full path to cfg>
                elif (len(line.split(',')) == 4 or len(line.split(',')) == 3) and not '#' in line:
                    
                    self._parseConfigGpj(line)    
                elif self._re_SetConstant.match(line):
                    pass
                else:
                    #Report excluded lines that were not comments
                    if not '#' in line and line.strip() != "":
                        self.notOnBuildCfgList.append(line.strip())
                    
            else:
                #Only report lines that were excluded for their buildGoal or chipSelect Option
                if not '#' in line and len(line) > 1:
                    self.notOnBuildCfgList.append(line.strip())
             
                
            #if self._stopOnError and len(self.buildErrors) > 0:
            #    sys.exit(1)
    
    
    def _checkBuildOptions(self, lineInput):
        """Function that determines if a line in a config file needs to be
        parsed.  It will return the input line with the build options removed and
        tell whether the line should be parsed.
        
        Return values:
        lineInput:      Line taken from command file with build options removed
        parseLine:      True --> Line get parsed, False --> Discard line
        
        """
        
        #Default action
        parseLine = False
        
        #Regular expression for finding goal and determing when to configure gpj    
        re_BuildGoal = re.compile('''[(](.+)[)]''')
        
        #Determine what needs to be removed
        buildGoalResult = re_BuildGoal.findall(lineInput)
        
        #Test to see if line is parsed
        if (len(buildGoalResult) == 1 or not ('(' in lineInput or ')' in lineInput)):
            #If parenthesis is used, test conditions to build
            if len(buildGoalResult) == 1:
                
                #Determine what format the user used for setting condition (<buildGoal> , <chipSelect>)
                #Defaults: <buildGoal> -> all_goals <chipSelect> ->  36xx
                if len(buildGoalResult[0].split(',')) >= 2:
                    buildConditions = buildGoalResult[0].split(',')[0].split('|') + buildGoalResult[0].split(',')[1].split('|')
                elif len(buildGoalResult[0].split(',')) == 1:
                    buildConditions = buildGoalResult[0].split(',')[0].split('|')
                    
                #Variables used to see if user utilize any of the condition variables, otherwise go to default
                if len(buildConditions) == 0:
                    chipSelectUsed = False
                    buildGoalUsed = False
                else:
                    chipSelectUsed = False
                    buildGoalUsed = True
                
                for i in range(0,len(buildConditions)):
                    buildConditions[i] = buildConditions[i].strip()
                    
                
                #Test to see if correct conditions are chosen for buildGoal and chipSelect
                if self._buildGoal.lower() in buildConditions or self._buildGoal.upper() in buildConditions or 'all_goals' in buildConditions or 'ALL_GOALS' in buildGoalResult[0] or not buildGoalUsed:
                    if self._chipSelect in buildConditions or 'all_chips' in buildConditions or 'ALL_CHIPS' in buildConditions or chipSelectUsed or self._chipSelect == 'all' or self._chipSelect == "ALL": 
                        parseLine = True
                        lineInput = lineInput.replace('(' + buildGoalResult[0] + ')','').strip()
            
            #If no conditions are listed, then build it automatically            
            elif len(buildGoalResult) == 0:
                if '#' in lineInput:
                    parseLine = False
                elif self._chipSelect in self._chipList:
                    parseLine = True
        
        return lineInput, parseLine
    
    
    
    def _parseGpj(self, lineInput):
        """Function parses out the path to the gpj and adds it to the list of
        gpjs to build.
        
        """
        
        #Check to see if correct path to gpj file
        if not os.path.exists(lineInput.strip()):
            self.buildErrors.append("Error: Could not find " + lineInput.strip())
        else:
            #Add path to projects that are going to be built    
            try:
                self.buildCfgList.append(os.path.abspath(lineInput.strip()))
            except Exception, e:
                self.buildErrors.append("Error: Could not include project file " + line.strip() + "\nDescription: " + str(e) + "\n\n")
            
    
    def _parseConfigGpj(self, lineInput, configureOnly=False, version=False):
        """Function parses lines with gpjs that need to be configured before
        being added to the projects to build list.
        
        A config gpj line can consist of four parts.  Each part should be
        separated by a comma.
        
        Parts:
        -- Relative or full path to gpj
        -- Releative or full path to tgpj
        -- Config options
        -- Relative or full path to a cfg file
        
        At minimum, the first two parts and at least one of the last two parts
        need to be used in a config gpj line.
        
        """
        #If using $$config syntax, erase it before proceeding
        if configureOnly:
            lineInput = lineInput.replace("$$configure", "")
        
        numArgs = len(lineInput.split(','))
                    
        #Get the paths for gpj and tgpj files
        gpjOutputPath = lineInput.split(',')[0].strip()
        tgpjInputPath = lineInput.split(',')[1].strip()

        #Determine the format used
        #If cfg file included or configs or both 
        if numArgs == 3 and '.cfg' in lineInput:
            configFilePath = lineInput.split(',')[2].strip()
            configOptions = []
        elif numArgs == 3 and not '.cfg' in lineInput:
            configOptions = (lineInput.split(',')[2]).split()
            configFilePath = None
        elif numArgs == 4:
            configOptions = (lineInput.split(',')[2]).split()
            configFilePath = lineInput.split(',')[3].strip()
        
        #Get initial count of errors, so you can compare later
        tempErrorCnt = len(self.buildErrors)
        
        #Check to see if tgpj path is correct
        if not os.path.exists(tgpjInputPath.strip()):
            self.buildErrors.append("Error: Could not find " + tgpjInputPath.strip())
            
        
        #Check to see if config file path is correct if using a cfg file
        if configFilePath:
            if not os.path.exists(configFilePath):
                self.buildErrors.append("Error: Could not find " + configFilePath)
        
        #If version option is selected, attempt to apply product and component version        
        if version:
            productVersionConfig = "-Dproduct_version=" + str(self._productVersion)
            componentVersionConfig = "-Dcomponent_version=" + str(self._componentVersion)
            
            configOptions.append(productVersionConfig)
            configOptions.append(componentVersionConfig)
        
        if len(self.buildErrors) == tempErrorCnt:                
            #Configure gpj file
            outputGpj, configs, deselectConfigs, properties  = \
                        self._configureGpj( gpjOutputPath.strip() ,tgpjInputPath.strip(),configOptions,configFilePath)
            
            if configureOnly:
                #Add to list of gpjs that are only being configured
                self.configureOnlyList.append(os.path.abspath(outputGpj))
            else:
                #Add gpj to list of projects needed to be built
                self.buildCfgList.append(os.path.abspath(outputGpj))
                
                
    def _parseProject(self, lineInput):
        """Function parses the $$project line and performs the steps necessary
        to generate a 37xx example player.
        
        Note: For right now, this function only performs step 4 where config
        text files are copied over to release folder.
        
        Player Config Steps:
        1) Generate the player, hostlink, and updater gpjs
        2) Add the 3 gpjs to the projects to build list in build_new_sdk.py
        3) Add the necessary files to the release folder copy list.
        4) Remove the generated config text files after we copied them to
           the release folder.
        
        Project Types:
        -pmptest
        -cinema
        -qa
        
        Text format for line:
        $$project <path to player and hostlink cfg> , <path to updater cfg>, <project type>
        
        """
        #Get rid of the $$player syntax
        lineInput = lineInput.replace("$$player", "")
        
        numArgs = len(lineInput.split(','))
        lineArgs = lineInput.split(',')
        
        #Verify that the number of arguments are correct for $$player syntax          
        if numArgs == 3 and type(lineArgs) == types.ListType:
            playerAndHostlinkCfgPath = lineArgs[0].strip()
            updaterCfgPath = lineArgs[1].strip()
            projectType = lineArgs[2].strip()
            projectName = os.path.basename(playerAndHostlinkCfgPath)[:-4]
            
        else:
            self.buildErrors.append("Error: Incorrect Format for $$project line: " + str(lineInput))
            return None
            
        #Determine project type
        if projectType.lower() in ["pmptest", "qa"]:
            projectGpjPath = os.path.join(self._rootDir, "application",
                                          "test", "pmp_application_test")
        elif projectType.lower() == "cinema":
            projectGpjPath = os.path.join(self._rootDir, "application",
                                          "examples", "players",
                                          "cinema")
        else:
            projectGpjPath = None
            self.buildErrors.append("Error: Incorrect player type: " + str(lineInput))
            return None
         
         
        #Add additional versioning properties to each gpj
        #These versions come from build_new_sdk.py
        productVersionConfig = "-Dproduct_version=" + str(self._productVersion)
        componentVersionConfig = "-Dcomponent_version=" + str(self._componentVersion)
            
        #Config the three project gpjs using one cfg file
        #Updater gpj
        updaterGpjOutputPath, updaterConfigs, updaterDeselectConfigs,\
            updaterProperties = self._configureGpj(
            os.path.join(projectGpjPath, projectName + "_updater_top.gpj"),
            self._updaterTemplatePath,
            [productVersionConfig, componentVersionConfig],
            updaterCfgPath, dryRun=True
        )
        #Hostlink gpj
        hostlinkGpjOutputPath, hostlinkConfigs, hostlinkDeselectConfigs,\
            hostlinkProperties = self._configureGpj(
            os.path.join(projectGpjPath, projectName + "_hostlink_top.gpj"),
            self._hostlinkTemplatePath,
            [productVersionConfig, componentVersionConfig],
            playerAndHostlinkCfgPath, dryRun=True
        )
        #Player gpj
        playerGpjOutputPath, playerConfigs, playerDeselectConfigs,\
            playerProperties = self._configureGpj(
            os.path.join(projectGpjPath, projectName + "_player_top.gpj"),
            self._playerTemplatePath,
            [productVersionConfig, componentVersionConfig],
            playerAndHostlinkCfgPath, dryRun=True
        )
        
        #Get name of project output folder
        playerProjectOutputFolder = playerProperties['project_name']
        updaterProjectOutputFolder = updaterProperties['project_name']
        
        #Add them to the build list
        #self.buildCfgList.append(os.path.abspath(updaterGpjOutputPath))
        #self.buildCfgList.append(os.path.abspath(hostlinkGpjOutputPath))
        #self.buildCfgList.append(os.path.abspath(playerGpjOutputPath))
        
        #Determine if the project is debug or a release build
        if "debug" in playerConfigs:
            projectDebugConfig = "debug"
        else:
            projectDebugConfig = "release"
            
        #Determine if project is sdram or a nosdram project
        if "sdram" in playerDeselectConfigs:
            memType = "nosdram"
        else:
            memType = "sdram"
        
        #Add the associated items to the copy list
        #playerProjectOutputPath = os.path.join(self._rootDir, "output", projectDebugConfig, memType, playerProjectOutputFolder)
        #updaterProjectOutputPath = os.path.join(self._rootDir, "output", projectDebugConfig, memType, updaterProjectOutputFolder)
        #self.copyFileDict[os.path.join(playerProjectOutputPath, "firmware.sb")] = ["##RELEASEDIR##/" + str(projectDebugConfig) + "/" + str(playerProjectOutputFolder) + "/firmware.sb"]
        
        #if os.path.join(updaterProjectOutputPath, "updater.sb") in self.copyFileDict.keys():
        #    self.copyFileDict[os.path.join(updaterProjectOutputPath, "updater.sb")].append("##RELEASEDIR##/" + str(projectDebugConfig) + "/" + str(playerProjectOutputFolder) + "/updater.sb")
        #else:
        #    self.copyFileDict[os.path.join(updaterProjectOutputPath, "updater.sb")] = ["##RELEASEDIR##/" + str(projectDebugConfig) + "/" + str(playerProjectOutputFolder) + "/updater.sb"]
        
        #Add a text file to the release folder with the player and hostlink config options
        playerHostlinkCfgDescTxtPath = self._genCfgTxt(
                                        str(projectName) + " Player and Hostlink",
                                        playerConfigs, playerDeselectConfigs, playerProperties)
        self.copyFileDict[playerHostlinkCfgDescTxtPath] = ["##RELEASEDIR##/" + projectDebugConfig + "/" + playerProjectOutputFolder + "/" + projectName + "_player_hostlink_config.txt"]
        self.cleanUpFilesList.append(playerHostlinkCfgDescTxtPath)
        
        #Add a text file to the release folder with the updater config options
        updaterCfgDescTxtPath = self._genCfgTxt(
                                        str(projectName) + " Updater",
                                        updaterConfigs, updaterDeselectConfigs, updaterProperties)
        self.copyFileDict[updaterCfgDescTxtPath] = ["##RELEASEDIR##/" + projectDebugConfig + "/" + playerProjectOutputFolder + "/" + projectName + "_updater_config.txt"]
        self.cleanUpFilesList.append(updaterCfgDescTxtPath)
    
    
    def _genCfgTxt(self, projectName, configs, deselectConfigs, properties):
        """Function creates a temporary text file with a description of
        the config options that make up a specified project.
        
        """
        
        tempFilePath = tempfile.mktemp()
        
        tempCfgFileDesc = open(tempFilePath, "w")
        tempCfgFileDesc.write("\nProject: " + str(projectName))
        tempCfgFileDesc.write("\nConfigs: " + str(configs))
        tempCfgFileDesc.write("\nDeselectConfigs: " + str(deselectConfigs))
        tempCfgFileDesc.write("\nProperty values: " + str(properties)+ "\n" )
        
        return tempCfgFileDesc.name
    
    
    def _parseCopy(self, lineInput):
        """Function parses a line from a command file and inserts the source
        and destination path into the copyFileDict dictionary.
        
        Both the source and destination paths should be relative to where
        the command text file is located.
        
        Syntax:
        $$copy <source file path> <destination file path>
        
        Built-in constants:
        ##RELEASEDIR## --> Path to release directory
                           Usual path: .\SOCFirmware\NNNN_NN_NN_NN_NN_NN
                           (N is a number)
        """
       
        if len(lineInput.strip().split()) == 3:
            sourcePath = os.path.abspath(lineInput.strip().split()[1])
    
            if not '##RELEASEDIR##' in lineInput:
                dstPath = os.path.abspath(lineInput.strip().split()[2])
            else:
                dstPath = lineInput.strip().split()[2]
            
            if self.copyFileDict:
                if sourcePath in self.copyFileDict.keys():
                    self.copyFileDict[sourcePath].append(dstPath)
                else:       
                    self.copyFileDict[sourcePath] = [dstPath]
            else:
                self.copyFileDict[sourcePath] = [dstPath]
                
        else:
            self.buildErrors.append("Error: Incorrect syntax " + lineInput.strip())
        

    def _parseInclude(self, lineInput, currentDir):
        """This function parses lines within the command file that contain the
        $$include sytax.  The path to an another text file will be parsed and
        an attempt will be made to open the file.  Once the file is open, the
        searchCommandFile will be called again to parse its contents.
        
        """
        oldDir = os.getcwd()
        
        try:
            
            file = lineInput.split('include')[1].strip()
            
            
            os.chdir(currentDir)
            file = os.path.abspath(file)
            
            fileName = os.path.split(file)[1]
            chgDir = os.path.split(file)[0]
            currDir = os.getcwd()
            
            #Make the location of the file the new current path
            os.chdir(chgDir)
            
            openExtFile = open(fileName,'r')
            #All the lines in the text file are dumped into a single string
            extTemp = openExtFile.read()
            
            openExtFile.close()
            
            #Convert multiline commands into a single line command
            extTempText = self._convertMultiLineCmd(extTemp)
            
            #Update list with project files
            self._searchCommandFile(extTempText, os.getcwd())
            
            
            openExtFile.close()
            
            #Change back to current dir
            os.chdir(oldDir)
            
        except Exception, e:
            #Ignore missing text file when goal=customer
            if not self._ignoreMissingTxtFile:
                self.buildErrors.append("Error: " + lineInput + " could not be opened!\nDescription: " + str(e) + "\n\n")
            
            os.chdir(oldDir)
  
  
    def _parseRun(self, lineInput):
        """Function parses a line from the command file and separates it into
        the executable file that needs to be run on the command line.
        
        Syntax used to indicate a command line:
        $$run <command line> (<build goal> , <chip select> )
        
        Built-in constants:
        ##BIN## --> Path to bin directory where build_new_sdk.py resides
                           Usual path: .\SOCFirmware\
        
        """
        
        if len(lineInput.split('$$run')) == 2:
            
            command = lineInput.replace("$$run", "").strip()
            
            command = command.replace("##BIN##", self.binDir)
            
            self.runCommandDict[command] = os.getcwd()
        else:
            self.buildErrors.append("Error: Incorrect $$run syntax\nLine: " + lineInput + "\n\n")
    
            
        
                
    def _configureGpj(self, gpjOutputPath, tgpjInputPath, configOptions,
                      configFilePath, dryRun=False):
        """Function configures a specified gpj file.
        
        """
        
        configs = []
        deselectConfigs = []
        properties = {}
        
        if configOptions:
            cfgLines = configOptions
        else:
            cfgLines = []
            
        #Parse properties and config info from config file
        if configFilePath:
           
            if os.path.exists(os.path.abspath(configFilePath)):
                try:
                    cfgFileText = open(os.path.abspath(configFilePath), 'r')
                    
                    cfgLines = cfgLines + cfgFileText.readlines()
                    
                except:
                    self.buildErrors.append("Error: Could not open cfg file " + configFilePath)
            
            else:
                self.buildErrors.append("Error: Cfg file " + configFilePath + " could not be found!")
            
                
               
        for line in cfgLines:
            if not '#' in line:
                #Properites passed through directly
                if '=' in line and '-D' in line:
                    #Split the -D option into property and its value
                    propertyName, propertyValue = line.split('=')
                    
                    #Remove -D from the name
                    newPropertyName = propertyName.replace("-D","")
                    
                    #Add the property to the current properties dictionary
                    properties[newPropertyName.strip()] = propertyValue.strip()
                
                #This is for properties passed through a cfg file
                elif '=' in line and not '-D' in line:
                    #Split the option into property and its value
                    propertyName, propertyValue = line.split('=')
                    
                    #Add the property to the current properties dictionary
                    properties[propertyName.strip()] = propertyValue.strip()
                    
                #Configs      
                elif not line.strip() == "" and not '=' in line:
                    #Config Names
                    configName= line.strip()
                    
                    if line.strip().startswith("!"):
                        deselectConfigs.append(configName.replace('!',''))
                    else:
                        configs.append(configName)
                            
        try:
            if not self._debugMode and not dryRun:
                configOp = config(os.path.abspath(tgpjInputPath), os.path.abspath(gpjOutputPath), configs, deselectConfigs, properties, overwrite=self._configUpdateMode, bLogTheConfig=self._configureOnly)
                configOp.run()
                
        except Exception, e:
            print e
            self.buildErrors.append("Config Error: " + gpjOutputPath + str(e)) 
                             
            if self._stopOnError and len(self.buildErrors):
                sys.exit(1)
              
        
        return gpjOutputPath, configs, deselectConfigs, properties
    
    
    def copyFiles(self, releaseDirPath):
        """Function copies files specified in the config text file to their
        destination.
        
        """
        
        for sourcePath in self.copyFileDict.keys():
            
            #If in configure mode, only copy the python files
            if (self._configureOnly and sourcePath.endswith(".py")) or not self._configureOnly:
            
                for dstPath in self.copyFileDict[sourcePath]:
                    
                    if "##RELEASEDIR##" in dstPath :
                        dstPath = dstPath.replace("##RELEASEDIR##", releaseDirPath)
                    
                    #Check to see if sourceFile exists before copying
                    if not os.path.exists(sourcePath):
                        self.buildErrors.append("Error: source path " + sourcePath + " not found!")
                    
                    else:
                        
                        if os.path.isdir(sourcePath):
                            try:
                                if dstPath.endswith('\\') or dstPath.endswith('/'):
                                    dstPath = dstPath[:-1]
    
                                if not os.path.exists(os.path.dirname(dstPath)):
                                    os.makedirs(os.path.dirname(dstPath))
                                
                                shutil.copytree(sourcePath,dstPath)
                                
                                self.log("\nItem: " + os.path.basename(sourcePath))
                                self.log("Source: " + sourcePath)
                                self.log("Destination: " + dstPath + "\n")
    
                            except:
                                self.buildErrors.append("Error: Unable to copy directory " + sourcePath)
    
                        elif os.path.isfile(sourcePath):
                            
                            try:
                                if not os.path.exists(os.path.dirname(dstPath)):
                                    os.makedirs(os.path.dirname(dstPath))
    
    
                                if not os.path.exists(dstPath) and not os.path.isfile(dstPath):
                                    shutil.copy(sourcePath, dstPath)
    
                                    self.log("\nItem: " + os.path.basename(sourcePath))
                                    self.log("Source: " + sourcePath)
                                    self.log("Destination: " + dstPath + "\n")
    
                            except:
                                self.buildErrors.append("Error: Unable to copy file " + sourcePath)
                         
                        else:
                            self.buildErrors.append("Error: source path " + sourcePath + " not found!")
                    
                    #if self._stopOnError and len(self.buildErrors):
                    #    sys.exit(1)
                    
        #Clean up cfg text files
        for file in self.cleanUpFilesList:
            try:
                os.remove(file)
            except Exception, e:
                self.buildErrors.append("Error: Unable to delete temp cfg text file: " + str(file))
    
    
    def parseTxtFile(self):
        """Function goes through the process of parsing the build text file
        and listing the gpjs that are going to be built and the the gpj projects
        ignored by the user goal settings.
        
        """
        
        self.log( "Build Text File       = " + str(self._commandFile) + "\n" )
        
        buildScriptDir = os.getcwd()
        commandFileDir = os.path.dirname(os.path.abspath(self._commandFile))
         
        #Open up the main command file (usually projects_to_build.txt file)
        commandFileText = self._openCommandFile()
        
        #Change working directory to location of text file.
        if commandFileText:
            os.chdir(commandFileDir)
        
        #Parse multi commands with brackets and convert them to single line
        #form.
        commandFileTextLines = self._convertMultiLineCmd(commandFileText)
        
        #Parse out text commands into different categories
        self._searchCommandFile(commandFileTextLines, commandFileDir)
        
        #Change back to previous working directory
        os.chdir(buildScriptDir)
        
        re_GpjFile = re.compile('''[A-Za-z0-9_.]+[.]gpj\s*''')
        
        filteredBuildCfgList = []
        for project in self.buildCfgList:
            filteredProjectName = re_GpjFile.findall(project)
            if filteredProjectName:
                filteredBuildCfgList.append(filteredProjectName[0])
                
        filteredConfigOnlyList = []
        for project in self.configureOnlyList:
            filteredProjectName = re_GpjFile.findall(project)
            if filteredProjectName:
                filteredConfigOnlyList.append(filteredProjectName[0])

        
        filteredBuildCfgList.sort()
        
        filteredConfigOnlyList.sort()
        
        
        #Print out projects that are configured and ready to be built
        if len(filteredBuildCfgList) > 0:
            self.log("\n\nProjects to build:")
            self.log("------------------\n")
        
            for project in filteredBuildCfgList:
                self.log(project)
        
        #Print out projects that are only going to be configured
        if len(filteredConfigOnlyList) > 0:
            self.log("\n\nProjects only configured:")
            self.log("----------------------------\n")
            
            for project in filteredConfigOnlyList:
                self.log(project)
        
        #Print out lines that were ignored during the parsing of all text files    
        if self._configureOnly:
            self.log("\n\nLines ignored in " + self._commandFile + ":")
            self.log("-------------------------------\n")
            for item in self.notOnBuildCfgList:
                self.log("* " + item)
            
        self.log("\n\n")
        
    
    def runExecutables(self):
        """Function executes all command line operations parsed from command file.
        
        TODO: Need to work on setting the actual working directory
        
        """
        
        self.runCommandDict
        
        for command in self.runCommandDict.keys():
            #Replace ##BIN## constant if present
            formattedCmd = command.replace("##BIN##", os.getcwd())
            
            self.setCommandPath(formattedCmd)
                
            self.log("\n" + "-"*80)
            self.log("Command: " + self.formatCommand())
            self.log("-"*80 + "\n")
                
            returnCode = self.copyOutputToLog(self.executeCommand())
            
            self.log("\n" + "-"*80)
            self.log("Return Code: " + returnCode)
            self.log("-"*80 + "\n")
        
                        
#//////////////////////////////////////////////////////////////////////////////
# Main function
#//////////////////////////////////////////////////////////////////////////////
def main():
    
    parseBuildTxt = ParseProjectsToBuildTxt()
    ParseProjectsToBuildTxt.userInterface()
    
#//////////////////////////////////////////////////////////////////////////////
# Start main function when run directly
#//////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
    main()        
