/** \file
 * \brief UIViewController hosting one IupDialog.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include "iup.h"

@class IupCocoaTouchFixed;

@interface IupViewController : UIViewController
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) UIScrollView* scrollView;
@property(nonatomic, retain) IupCocoaTouchFixed* clientArea;
@property(nonatomic, assign) BOOL hideStatusBar;
@property(nonatomic, assign) BOOL hideHomeIndicator;
@property(nonatomic, assign) UIInterfaceOrientationMask allowedOrientations;
@end
