#import "ViewController.h"
#import "AppDelegate.h"

@interface ViewController () {
  AppDelegate *appd_;
}
@end

@implementation ViewController

@synthesize frequencyLabel;

- (id)init {
  self = [super init];
  if (self) {
    appd_ = nil;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  appd_ = [[NSApplication sharedApplication] delegate];
}

- (void)setRepresentedObject:(id)representedObject {
  [super setRepresentedObject:representedObject];

  // Update the view, if already loaded.
}

- (IBAction)waveformAction:(NSPopUpButton *)sender {
  if (appd_ != nil) {
    [appd_ setWaveform:sender.selectedTag];
  }
}

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
