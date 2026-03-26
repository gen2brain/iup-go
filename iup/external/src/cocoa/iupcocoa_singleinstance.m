/** \file
 * \brief macOS Single Instance Support - CFMessagePort Implementation
 *
 * This file provides single instance detection using CFMessagePort
 * for the Cocoa driver and for GTK/Qt/EFL running on macOS.
 *
 * When the first instance calls iupdrvSingleInstanceSet(), it creates
 * a local CFMessagePort with a well-known name and installs it in the
 * run loop to receive data from subsequent instances.
 *
 * When a second instance calls iupdrvSingleInstanceSet(), it detects
 * that the port name is already in use, sends the command-line arguments
 * via the existing remote port, and returns 1.
 *
 * The first instance delivers received data to the first dialog
 * that has COPYDATA_CB registered, matching the Windows behavior.
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dlglist.h"

#include "iup_singleinstance.h"

static CFMessagePortRef si_local_port = NULL;
static CFRunLoopSourceRef si_run_loop_source = NULL;

static void iupCocoaSIDeliverData(const char* data, int len)
{
  Ihandle* dlg;

  for (dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    IFnsi cb = (IFnsi)IupGetCallback(dlg, "COPYDATA_CB");
    if (cb)
    {
      cb(dlg, (char*)data, len);
      return;
    }
  }
}

static CFDataRef iupCocoaSIPortCallback(CFMessagePortRef local, SInt32 msgid, CFDataRef cfdata, void* info)
{
  (void)local;
  (void)msgid;
  (void)info;

  if (cfdata)
  {
    const char* data = (const char*)CFDataGetBytePtr(cfdata);
    int len = (int)CFDataGetLength(cfdata);

    if (data && len > 0)
      iupCocoaSIDeliverData(data, len);
  }

  [NSApp activateIgnoringOtherApps:YES];

  return NULL;
}

static char* iupCocoaSIGetCommandLine(void)
{
  NSProcessInfo* info = [NSProcessInfo processInfo];
  NSArray<NSString*>* args = [info arguments];
  NSString* joined = [args componentsJoinedByString:@" "];
  const char* utf8 = [joined UTF8String];

  if (utf8 && utf8[0])
    return iupStrDup(utf8);

  return iupStrDup("");
}

static void iupCocoaSISendToFirst(CFStringRef port_name)
{
  CFMessagePortRef remote;
  char* cmdline;
  CFDataRef cfdata;

  remote = CFMessagePortCreateRemote(kCFAllocatorDefault, port_name);
  if (!remote)
    return;

  cmdline = iupCocoaSIGetCommandLine();

  cfdata = CFDataCreate(kCFAllocatorDefault, (const UInt8*)cmdline, (CFIndex)strlen(cmdline) + 1);
  if (cfdata)
  {
    CFMessagePortSendRequest(remote, 0, cfdata, 2.0, 0, NULL, NULL);
    CFRelease(cfdata);
  }

  CFRelease(remote);
  free(cmdline);
}

IUP_SDK_API int iupdrvSingleInstanceSet(const char* name)
{
  CFStringRef port_name;
  Boolean should_free;
  CFMessagePortContext context = { 0, NULL, NULL, NULL, NULL };

  if (!name || !name[0])
    return 0;

  port_name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("org.iup.si.%s"), name);
  if (!port_name)
    return 0;

  si_local_port = CFMessagePortCreateLocal(kCFAllocatorDefault, port_name, iupCocoaSIPortCallback, &context, &should_free);

  if (!si_local_port || should_free)
  {
    iupCocoaSISendToFirst(port_name);
    CFRelease(port_name);
    return 1;
  }

  si_run_loop_source = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, si_local_port, 0);
  if (si_run_loop_source)
    CFRunLoopAddSource(CFRunLoopGetMain(), si_run_loop_source, kCFRunLoopCommonModes);

  CFRelease(port_name);
  return 0;
}

IUP_SDK_API void iupdrvSingleInstanceClose(void)
{
  if (si_run_loop_source)
  {
    CFRunLoopRemoveSource(CFRunLoopGetMain(), si_run_loop_source, kCFRunLoopCommonModes);
    CFRelease(si_run_loop_source);
    si_run_loop_source = NULL;
  }

  if (si_local_port)
  {
    CFMessagePortInvalidate(si_local_port);
    CFRelease(si_local_port);
    si_local_port = NULL;
  }
}
