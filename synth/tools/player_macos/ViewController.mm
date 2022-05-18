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
  UInt16 prevActive_;
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

  NSSegmentedControl *const v = voices;
  for (NSInteger segment = 0, lastSegment = v.segmentCount; segment < lastSegment; ++segment) {
    [v setImage:inactiveImage_ forSegment:segment];
  }
#elif TARGET_OS_IOS
  appd_ = static_cast<AppDelegate *> ([[UIApplication sharedApplication] delegate]);
//  UIImageConfiguration activeConfiguration;
//  activeImage_ = [UIImage systemImageNamed:@"circle.fill" withConfiguration:&activeConfiguration];
//  UIImageConfiguration inactiveConfiguration;
//  inactiveImage_ = [UIImage systemImageNamed:@"circle.fill"
//  withConfiguration:&inactiveConfiguration];
#endif

  prevActive_ = 0U;
  [[NSRunLoop currentRunLoop]
      addTimer:[[NSTimer alloc] initWithFireDate:[NSDate dateWithTimeIntervalSinceNow:1.0]
                                        interval:0.1
                                          target:self
                                        selector:@selector (updateActiveVoices)
                                        userInfo:nil
                                         repeats:YES]
       forMode:NSDefaultRunLoopMode];
}

- (void)updateActiveVoices {
  UInt16 const active = [appd_ activeVoices];
  if (active == prevActive_) {
    return;
  }
  prevActive_ = active;
  NSSegmentedControl *const v = voices;
  for (NSInteger segment = 0, lastSegment = v.segmentCount; segment < lastSegment; ++segment) {
    NSImage *const image = (active & (1U << segment)) != 0 ? activeImage_ : inactiveImage_;
    [v setImage:image forSegment:segment];
  }
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
    using phase = synth::envelope<sample_rate>::phase;
    auto stage = phase::idle;
    switch (sender.tag) {
      case 0:
        stage = phase::attack;
        break;
      case 1:
        stage = phase::decay;
        break;
      case 2:
        stage = phase::sustain;
        break;
      case 3:
        stage = phase::release;
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
