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
- (UInt16)activeVoices;

@end

#endif  // APP_DELEGATE_H
