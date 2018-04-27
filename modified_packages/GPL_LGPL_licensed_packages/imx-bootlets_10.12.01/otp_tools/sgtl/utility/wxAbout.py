#Boa:Dialog:About

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

import wx

def create(parent):
    return About(parent)

[wxID_ABOUT, wxID_ABOUTABOUTOK, wxID_ABOUTCOMMENTS, wxID_ABOUTPRODUCTNAME,
 wxID_ABOUTVERSION,
] = [wx.NewId() for _init_ctrls in range(5)]

class About(wx.Dialog):
    ProductStr  = None
    VersionStr  = None
    CommentStr  = "Copyright (c) Freescale Semiconductor, Inc."

    def _init_coll_boxSizer1_Items(self, parent):
        # generated method, don't edit

        parent.AddWindow(self.ProductName, 0, border=0, flag=0)
        parent.AddWindow(self.Version, 0, border=0, flag=0)
        parent.AddWindow(self.Comments, 0, border=0, flag=0)

    def _init_sizers(self):
        # generated method, don't edit
        self.boxSizer1 = wx.BoxSizer(orient=wx.VERTICAL)

        self._init_coll_boxSizer1_Items(self.boxSizer1)

        self.SetSizer(self.boxSizer1)

    def _init_ctrls(self, prnt):
        # generated method, don't edit
        wx.Dialog.__init__(self, id=wxID_ABOUT, name='About', parent=prnt,
              pos=wx.Point(527, 463), size=wx.Size(297, 169),
              style=wx.DEFAULT_DIALOG_STYLE, title='About')
        self.SetClientSize(wx.Size(289, 135))

        self.AboutOk = wx.Button(id=wxID_ABOUTABOUTOK, label='OK',
              name='AboutOk', parent=self, pos=wx.Point(107, 96),
              size=wx.Size(75, 25), style=0)
        self.AboutOk.Center(wx.HORIZONTAL)
        self.AboutOk.SetFont(wx.Font(10, wx.SWISS, wx.NORMAL, wx.NORMAL, False,
              'MS Shell Dlg 2'))
        self.AboutOk.Bind(wx.EVT_BUTTON, self.OnAboutOkButton,
              id=wxID_ABOUTABOUTOK)

        self.ProductName = wx.StaticText(id=wxID_ABOUTPRODUCTNAME, label='',
              name='ProductStr', parent=self, pos=wx.Point(0, 0),
              size=wx.Size(288, 25), style=wx.ALIGN_CENTER | wx.ST_NO_AUTORESIZE)
        self.ProductName.SetFont(wx.Font(14, wx.SWISS, wx.NORMAL, wx.NORMAL,
              False, 'MS Shell Dlg 2'))
        self.ProductName.SetToolTipString('')

        self.Version = wx.StaticText(id=wxID_ABOUTVERSION, label='',
              name='VersionStr', parent=self, pos=wx.Point(0, 25),
              size=wx.Size(288, 25), style=wx.ALIGN_CENTER | wx.ST_NO_AUTORESIZE)
        self.Version.SetFont(wx.Font(10, wx.SWISS, wx.NORMAL, wx.NORMAL, False,
              'MS Shell Dlg 2'))
        self.Version.SetToolTipString('')

        self.Comments = wx.StaticText(id=wxID_ABOUTCOMMENTS, label='',
              name='CommentStr', parent=self, pos=wx.Point(0, 50),
              size=wx.Size(288, 38), style=wx.ALIGN_CENTER | wx.ST_NO_AUTORESIZE)
        self.Comments.SetFont(wx.Font(10, wx.SWISS, wx.NORMAL, wx.NORMAL, False,
              'MS Shell Dlg 2'))
        self.Comments.SetToolTipString('')

        self._init_sizers()

    def __init__(self, parent):
        self._init_ctrls(parent)

    def OnAboutOkButton(self, event):
        event.Skip()
        self.Close()

    def UpdateFields(self):
        if self.ProductStr != None:
            self.ProductName.SetLabel(self.ProductStr.center(40))
        if self.VersionStr != None:
            self.Version.SetLabel (self.VersionStr.center(70))
        if self.CommentStr != None:
            self.Comments.SetLabel (self.CommentStr)
        self.Refresh()
        return

    def ModifyFields(self, productStr, versionStr=None, commentStr=None):
        self.ProductStr = productStr
        self.VersionStr = versionStr
        self.CommentStr = commentStr
        self.UpdateFields()
        return

    def Display(self,prodStr="",verStr="",ComStr=""):
        self.ProductStr = prodStr
        self.VersionStr = verStr
        self.CommentStr = ComStr
        self.UpdateFields()
        self.CenterOnScreen()
        val = self.ShowModal()  # this does not return until the dialog is closed.

#//////////////////////////////////////////////////////////////////////////////
# Start main function when run directly
#//////////////////////////////////////////////////////////////////////////////
class _BoaApp(wx.App):
    def OnInit(self):
        self.main = create(None)
        self.main.Show()
        self.SetTopWindow(self.main)
        return True

    def Update(self,name,ver,comments):
        self.main.ModifyFields (name, ver, comments)
        return

def _main():
    app = _BoaApp(0)
    app.Update("Test App", "1.2.3", "Comments section goes here")
    app.MainLoop()

if __name__ == '__main__':
    _main()
