/** \file
 * \brief iOS UIApplication delegate wiring the IUP entry point.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

/* host-app delegate hook so IUP can locate the active UIWindow */
@protocol IupAppDelegateProtocol <UIApplicationDelegate>
- (UIWindow*)currentWindow;
@end

@interface IupAppDelegate : UIResponder <IupAppDelegateProtocol>
@property(strong, nonatomic) UIWindow* window;
@end
