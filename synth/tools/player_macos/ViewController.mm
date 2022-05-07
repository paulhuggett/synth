#import "ViewController.h"
#import "AppDelegate.h"

@interface ViewController () {
  AppDelegate *_Nullable appd_;

#if TARGET_OS_OSX
  NSImage *activeImage_;
  NSImage *inactiveImage_;
#elif TARGET_OS_IOS
  UIImage *activeImage_;
  UIImage *inactiveImage_;
#endif
}
@end

@implementation ViewController

@synthesize frequencyLabel;
@synthesize voices;

// view did load
// ~~~~~~~~~~~~~
- (void)viewDidLoad {
  [super viewDidLoad];
#if TARGET_OS_OSX
  appd_ = [[NSApplication sharedApplication] delegate];
  activeImage_ = [NSImage imageNamed:NSImageNameStatusAvailable];
  inactiveImage_ = [NSImage imageNamed:NSImageNameStatusUnavailable];
  for (int segment = 0, lastSegment = voices.segmentCount; segment < lastSegment; ++segment) {
    [voices setImage:inactiveImage_ forSegment:segment];
  }
#elif TARGET_OS_IOS
  appd_ = static_cast<AppDelegate *> ([[UIApplication sharedApplication] delegate]);
//  UIImageConfiguration activeConfiguration;
//  activeImage_ = [UIImage systemImageNamed:@"circle.fill" withConfiguration:&activeConfiguration];
//  UIImageConfiguration inactiveConfiguration;
//  inactiveImage_ = [UIImage systemImageNamed:@"circle.fill"
//  withConfiguration:&inactiveConfiguration];
#endif
}

// set represented object
// ~~~~~~~~~~~~~~~~~~~~~~
#if TARGET_OS_OSX
- (void)setRepresentedObject:(id)representedObject {
  [super setRepresentedObject:representedObject];

  // Update the view, if already loaded.
}
#endif

#if TARGET_OS_OSX
// waveform action
// ~~~~~~~~~~~~~~~
- (IBAction)waveformAction:(NSPopUpButton *)sender {
  if (appd_ != nil) {
    [appd_ setWaveform:sender.selectedTag];
  }
}

// frequency action
// ~~~~~~~~~~~~~~~~
- (IBAction)frequencyAction:(NSSlider *)sender {
  double value = sender.doubleValue;
  NSString *unit = @"Hz";
  if (value > 5000.0) {
    unit = @"kHz";
    value /= 1000.0;
  }
  frequencyLabel.stringValue = [NSString stringWithFormat:@"%.3lf%@", value, unit];
  if (appd_ != nil) {
    [appd_ setFrequency:sender.doubleValue];
  }
}

// ADSR action
// ~~~~~~~~~~~
// Called when the value of one of the envelope sliders is changed.
- (IBAction)adsrAction:(NSSlider *)sender {
  if (appd_ != nil) {
    auto stage = synth::envelope::phase::idle;
    switch (sender.tag) {
      case 0:
        stage = synth::envelope::phase::attack;
        break;
      case 1:
        stage = synth::envelope::phase::decay;
        break;
      case 2:
        stage = synth::envelope::phase::sustain;
        break;
      case 3:
        stage = synth::envelope::phase::release;
        break;
    }
    [appd_ setEnvelopeStage:stage to:sender.doubleValue];
  }
}

#elif TARGET_OS_IOS

// waveform action
// ~~~~~~~~~~~~~~~
- (IBAction)waveformAction:(UISegmentedControl *)sender {
  if (appd_ != nil) {
  }
}

// frequency action
// ~~~~~~~~~~~~~~~~
- (IBAction)frequencyAction:(UISlider *)sender {
  float value = sender.value;
  NSString *unit = @"Hz";
  if (value > 5000.0F) {
    unit = @"kHz";
    value /= 1000.0F;
  }
  frequencyLabel.text = [NSString stringWithFormat:@"%.3lf%@", (double)value, unit];
  if (appd_ != nil) {
    [appd_ setFrequency:(double)sender.value];
  }
}

#endif

@end
