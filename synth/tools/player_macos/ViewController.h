#ifndef VIEW_CONTROLLER_H
#define VIEW_CONTROLLER_H

#import <TargetConditionals.h>

#if TARGET_OS_OSX

#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController
- (IBAction)waveformAction:(NSPopUpButton *)sender;
@property (nonatomic, weak) IBOutlet NSTextField *frequencyLabel;
@end

#elif TARGET_OS_IOS

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController
- (IBAction)waveformAction:(UISegmentedControl *)sender;
@property (nonatomic, weak) IBOutlet UITextField *frequencyLabel;
@end

#else
#error "Unknown Target"
#endif

#endif  // VIEW_CONTROLLER_H
