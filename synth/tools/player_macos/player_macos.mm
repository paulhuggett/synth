#import <TargetConditionals.h>

#if TARGET_OS_OSX

#import <Cocoa/Cocoa.h>

int main (int argc, const char *argv[]) {
  @autoreleasepool {
    // Setup code that might create autoreleased objects goes here.
  }
  return NSApplicationMain (argc, argv);
}

#elif TARGET_OS_IOS

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main (int argc, char* argv[]) {
  NSString* appDelegateClassName = nil;
  @autoreleasepool {
    // Setup code that might create autoreleased objects goes here.
    appDelegateClassName = NSStringFromClass ([AppDelegate class]);
  }
  return UIApplicationMain (argc, argv, nil, appDelegateClassName);
}

#else
#error "Unknown target"
#endif
