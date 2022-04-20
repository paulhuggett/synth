#ifndef APP_DELEGATE_H
#define APP_DELEGATE_H

#import <TargetConditionals.h>

#if TARGET_OS_OSX

#import <Cocoa/Cocoa.h>
@interface AppDelegate : NSObject <NSApplicationDelegate>

#elif TARGET_OS_IOS

#import <UIKit/UIKit.h>
@interface AppDelegate : UIResponder <UIApplicationDelegate>

#else
#error "Unknown target"
#endif

- (void)setWaveform:(NSInteger)which;
- (void)setFrequency:(double)f;

@end

#endif  // APP_DELEGATE_H
