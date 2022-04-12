#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController

- (IBAction)waveformAction:(NSPopUpButton *)sender;

@property (nonatomic, weak) IBOutlet NSTextField *frequencyLabel;

@end
