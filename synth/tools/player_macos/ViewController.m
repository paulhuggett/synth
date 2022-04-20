#import "ViewController.h"
#import "AppDelegate.h"

@interface ViewController () {
  AppDelegate *_Nullable appd_;
}
@end

@implementation ViewController

@synthesize frequencyLabel;

// init
// ~~~~
- (id)init {
  self = [super init];
  if (self) {
    appd_ = nil;
  }
  return self;
}

// view did load
// ~~~~~~~~~~~~~
- (void)viewDidLoad {
  [super viewDidLoad];
#if TARGET_OS_OSX
  appd_ = [[NSApplication sharedApplication] delegate];
#elif TARGET_OS_IOS
  appd_ = [[UIApplication sharedApplication] delegate];
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
