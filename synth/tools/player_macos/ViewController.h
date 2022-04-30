#ifndef VIEW_CONTROLLER_H
#define VIEW_CONTROLLER_H

#import <TargetConditionals.h>

#if TARGET_OS_OSX

#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController

- (IBAction)waveformAction:(NSPopUpButton *)sender;
- (IBAction)adsrAction:(NSSlider *)sender;
@property (nonatomic, weak) IBOutlet NSTextField *frequencyLabel;
@property (nonatomic, weak) IBOutlet NSSegmentedControl *voices;

@end

#elif TARGET_OS_IOS

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

- (IBAction)waveformAction:(UISegmentedControl *)sender;
- (IBAction)adsrAction:(UISlider *)sender;
@property (nonatomic, weak) IBOutlet UITextField *frequencyLabel;
@property (nonatomic, weak) IBOutlet UITextField *voices;

@end

#else
#error "Unknown Target"
#endif

#endif  // VIEW_CONTROLLER_H
