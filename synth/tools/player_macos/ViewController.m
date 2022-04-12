#import "ViewController.h"
#import "AppDelegate.h"

@interface ViewController () {
  AppDelegate *appd_;
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
  appd_ = [[NSApplication sharedApplication] delegate];
}

// set represented object
// ~~~~~~~~~~~~~~~~~~~~~~
- (void)setRepresentedObject:(id)representedObject {
  [super setRepresentedObject:representedObject];

  // Update the view, if already loaded.
}

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

@end
