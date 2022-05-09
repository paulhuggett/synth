#ifndef APP_DELEGATE_H
#define APP_DELEGATE_H

#import <TargetConditionals.h>
#include "synth/envelope.hpp"

#if TARGET_OS_OSX

enum EnvelopeStage { attack, decay, sustain, release };

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
- (void)setEnvelopeStage:(synth::envelope::phase)stage to:(double)value;

/// Returns a bitmask which describes the active voices at the time of being called. Bit 0 (LSB)
/// corresponds to the first voice, bit 1 to the second, and so on.
- (UInt16)activeVoices;

@end

#endif  // APP_DELEGATE_H
