// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef MSGBOX_H
#define MSGBOX_H


#include <stdio.h>
#include <stdlib.h>
#include <Xm/XmAll.h>


void ShowMessageBox(Widget, const char *, const char *);
int OkCancelDialog(Widget, const char *, const char *);
void OkCancelResponse(Widget, XtPointer, XtPointer);


#endif // MSGBOX_H
