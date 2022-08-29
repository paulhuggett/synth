#ifndef MIDI_H
#define MIDI_H

#import <AudioToolbox/AudioToolbox.h>
#import <TargetConditionals.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#elif TARGET_OS_IOS
#import <UIKit/UIKit.h>
#else
#error "Unknown target"
#endif

@interface MIDIChangeHandler : NSObject

- (id)init;
- (OSStatus)startMIDIWithReadProc:(MIDIReadProc)readProc refCon:(void *)refCon;

@end

#endif  // MIDI_H
