#ifndef SCOPE_VIEW_H
#define SCOPE_VIEW_H

#import <TargetConditionals.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
typedef NSView ViewBase;
#elif TARGET_OS_IOS
#import <UIKit/UIKit.h>
typedef UIView ViewBase;
#else
#error "Unknown target"
#endif

@interface ScopeView : ViewBase
@end

#endif  // SCOPE_VIEW_H
