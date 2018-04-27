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

## \ingroup utilities
# @file bootnow.py
#
# Interface to BootIT NG's BootNow utility.

import logging, os, subprocess, time

## Default path to the folder where BootIT NG tools are installed.
BasePath = os.path.join('D:', os.path.sep, 'BootNow')

## Executable filename for BootNow.
Executable = 'BOOTNOW.exe'

## Triggers reboot to the specified boot menu item using BootNow for BootIT.
# @param BootTarget: Numerical index or string with the label of the desired BootIT boot menu item.
# @param SecondsToDelay: Number of seconds to delay before issuing the reboot command.
# @note: The delay is blocking. This should not be used in place of postponing reboot until after any cleanup steps!
# @param Force: Whether to instruct BootNow to preempt dialogs that block the reboot waiting for user responses.
def BootNow(BootTarget = None, SecondsToDelay = 10, Force = False):
    # Begin building the command line for BootNow.exe 
    Command = '"%s"' % os.path.join(BasePath, Executable)

    Args = ''
    if BootTarget is not None:
        Args += str(BootTarget)
    if Force:
        Args += '/force'
    if len(Args) > 0:
        Command += ' ' + Args
    logging.debug('reboot command will be %s' % Command)

    # Optionally delay.
    if SecondsToDelay is not None and SecondsToDelay > 0:
        logging.info('rebooting in %u seconds' % SecondsToDelay)
        time.sleep(SecondsToDelay)

    # Launch the reboot utility.
    logging.info('rebooting now!')
    subprocess.Popen(Command)

# eof
