/** \file
 * \brief Keep the display awake
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>

#include "iup.h"

#include "iup_drvinfo.h"


static id cocoa_keepawake_activity = nil;

IUP_SDK_API void iupdrvSetKeepAwake(int keep)
{
  if (keep && !cocoa_keepawake_activity)
    cocoa_keepawake_activity = [[[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityIdleDisplaySleepDisabled | NSActivityIdleSystemSleepDisabled reason:@"Keep awake"] retain];
  else if (!keep && cocoa_keepawake_activity)
  {
    [[NSProcessInfo processInfo] endActivity:cocoa_keepawake_activity];
    [cocoa_keepawake_activity release];
    cocoa_keepawake_activity = nil;
  }
}
