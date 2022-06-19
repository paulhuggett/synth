#ifndef SCOPE_VIEW_H
#define SCOPE_VIEW_H

#import <TargetConditionals.h>

#if TARGET_OS_OSX

#import <Cocoa/Cocoa.h>
@interface ScopeView : NSView
@end

#elif TARGET_OS_IOS

#import <UIKit/UIKit.h>
@interface ScopeView : UIView
@end

#else
#error "Unknown target"
#endif

#endif  // SCOPE_VIEW_H
