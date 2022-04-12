#import "ViewController.h"
#import "AppDelegate.h"

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  // Do any additional setup after loading the view.
}

- (void)setRepresentedObject:(id)representedObject {
  [super setRepresentedObject:representedObject];

  // Update the view, if already loaded.
}

- (IBAction)waveformAction:(NSPopUpButton *)sender {
  NSInteger const tag = [sender selectedTag];
  NSLog (@"waveform action: %@ index:%ld tag:%ld", [sender selectedItem].title,
         [sender indexOfSelectedItem], tag);
  AppDelegate *const d = [[NSApplication sharedApplication] delegate];
  [d setWaveform:tag];
}

- (IBAction)frequencyAction:(NSSlider *)sender {
  AppDelegate *const d = [[NSApplication sharedApplication] delegate];
  [d setFrequency:sender.doubleValue];
}

@end
